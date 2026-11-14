# frozen_string_literal: true

# Part of Nexus Library
#
# `Nxs::Diff`, the changes between two renders of one page, marked in the newer one: blocks that
# are new, changed (with the words that changed) or gone (struck through, in place). Used by the
# `diff` plugin under `tools/doc.sh --diff`; pure: two HTML strings in, one out.
#
#   html, stats = Nxs::Diff.apply(new_html, old_html, ref: 'HEAD', head: '<style>..', body: '<script>..')
#
# `old_html` nil is a page that is new: a badge at its title, no marks on its blocks. Without any
# change, `apply` returns `new_html` as it is.
#
# The pages are compared block by block: headings, paragraphs, list items, terms and descriptions,
# table rows, admonitions, listings, figures. A block is its text with the inline markup that
# carries meaning (emphasis, code, link targets); white space is collapsed, so a source rewrapped
# compares equal, and the numbers of captions and sections are left out, so a figure inserted
# before others does not change them all. A block that cannot be read as text (an SVG, an image)
# compares by its markup. The blocks of both pages are aligned by their longest common sequence;
# between two aligned blocks, a removed and an inserted block of the same kind and with mostly the
# same words are one changed block, the rest are gone or new. A container whose blocks are all new
# (a section, a list, a table) is marked once, as is one whose blocks are all gone.

require 'nokogiri'
require 'zlib'

module Nxs
  module Diff
    # A block of a page: the element that stands for it in the tree (`el`), the one that carries
    # its mark (`mark`), its kind, its comparison key, its words; after the alignment, its status
    # (:same, :changed, :added, :deleted) and its counterpart in the other page (`peer`)
    Block = Struct.new(:el, :mark, :kind, :key, :words, :status, :peer)

    # Containers walked into: their own blocks are their children
    WALK = %w[sect0 sect1 sect2 sect3 sect4 sect5 sect6 sectionbody partintro exampleblock sidebarblock
              openblock content].freeze
    LISTS = %w[ulist olist colist dlist qlist].freeze
    # Containers that, all new or all gone, are marked once
    UNITS = %w[sect0 sect1 sect2 sect3 sect4 sect5 sect6 ulist olist colist dlist qlist hdlist exampleblock
               sidebarblock openblock quoteblock admonitionblock].freeze
    # Inline elements kept in the comparison key; others count by their text only
    MARKUP = %w[strong em code mark b i u s sub sup kbd var].freeze
    # Elements compared by their markup
    OPAQUE = %w[svg img video audio canvas iframe object math script].freeze
    HEADINGS = %w[h1 h2 h3 h4 h5 h6].freeze
    # Kinds of block where a removed and an inserted one are a change, whatever their words
    WHOLE = %w[imageblock listingblock literalblock stemblock videoblock audioblock].freeze
    SIMILAR = 0.5
    # The label and number of a caption, `Table 3. `, left out of the comparison
    CAPTION = /\A[[:alpha:]]+ [\dA-Z]+(\.\d+)*\.\s/

    module_function

    # The new page with the changes from the old one marked; and the counts:
    # `{ added:, changed:, deleted:, new_page: }`
    def apply(new_html, old_html, ref: 'HEAD', head: '', body: '', pages: [])
      doc = Nokogiri::HTML5(new_html)
      stats = { added: 0, changed: 0, deleted: 0, new_page: old_html.nil? }
      if old_html.nil?
        mark_new_page(doc)
      else
        mark_changes(doc, Nokogiri::HTML5(old_html), stats)
      end
      flag_pages(doc, pages)
      return [new_html, stats] unless changes?(stats) || !pages.empty?

      finish(doc, stats, ref, head, body, pages)
      [doc.to_html, stats]
    end

    def changes?(stats)
      stats[:new_page] || stats.values_at(:added, :changed, :deleted).sum.positive?
    end

    # ---------------------------------------------------------------------------------- Blocks

    def first_class(el)
      el['class'].to_s.split.first.to_s
    end

    # The blocks of a page's content, in document order
    def blocks(doc)
      out = []
      content = doc.at_css('#content') or return out
      walk(content, out)
      out
    end

    def walk(node, out)
      node.element_children.each { |el| visit(el, out) }
    end

    def visit(el, out)
      cls = first_class(el)
      if el['id'] == 'preamble'
        walk(el, out)
      elsif cls == 'toc' || el['id'] == 'toc'
        nil
      elsif HEADINGS.include?(el.name)
        out << block(el, el, 'heading')
      elsif el.name == 'div' && cls == 'title'
        out << block(el, el, 'title')
      elsif el.name == 'div' && WALK.include?(cls)
        walk(el, out)
      elsif el.name == 'div' && LISTS.include?(cls)
        list(el, out)
      elsif el.name == 'div' && cls == 'paragraph'
        out << block(el, el.at_xpath('./p') || el, 'paragraph')
      elsif el.name == 'table'
        table(el, out)
      elsif el.name == 'div' && cls == 'hdlist'
        el.element_children.each { |child| child.name == 'table' ? table(child, out) : visit(child, out) }
      elsif el.name == 'div' && cls == 'admonitionblock'
        out << block(el, el, 'admonition')
      else
        out << block(el, el, cls.empty? ? el.name : cls)
      end
    end

    # A list: its title, then each item (its own text, then the blocks nested in it)
    def list(el, out)
      el.element_children.each do |child|
        if first_class(child) == 'title'
          out << block(child, child, 'title')
        elsif %w[ul ol].include?(child.name)
          child.xpath('./li').each { |li| item(li, out) }
        elsif child.name == 'dl'
          child.element_children.each do |term|
            if term.name == 'dt' then out << block(term, term, 'term')
            elsif term.name == 'dd' then item(term, out)
            end
          end
        elsif child.name == 'table' # a colist as a table
          table(child, out)
        end
      end
    end

    def item(li, out)
      text = li.at_xpath('./p')
      out << block(li, text, li.name == 'dd' ? 'description' : 'item') if text
      li.element_children.each { |child| visit(child, out) unless child == text }
    end

    # A table: its caption, then each row
    def table(el, out)
      el.xpath('./caption | ./thead/tr | ./tbody/tr | ./tfoot/tr | ./tr').each do |row|
        out << (row.name == 'caption' ? block(row, row, 'title') : block(row, row, 'row'))
      end
    end

    def block(el, mark, kind)
      text = key(%w[item description].include?(kind) ? mark : el, kind)
      Block.new(el, mark, kind, text, words(text), nil, nil)
    end

    # ------------------------------------------------------------------------------------- Keys

    def key(el, kind)
      buf = +''
      sig(el, buf)
      text = squash(buf)
      text = text.sub(/\A(\d+\.)+\s/, '') if kind == 'heading'
      text = text.sub(CAPTION, '') if kind == 'title'
      text
    end

    def squash(text)
      text.gsub(/\s+/, ' ').strip
    end

    # The text of a node with the markup that matters: inline elements as `\u0001name ... \u0002`,
    # links with their targets, opaque elements as a checksum of their markup
    def sig(node, buf)
      node.children.each do |c|
        if c.text?
          buf << c.text
        elsif c.element?
          name = c.name
          cls = c['class'].to_s
          if name == 'sup' && cls.include?('footnote') || name == 'a' && cls.include?('anchor')
            next
          elsif OPAQUE.include?(name)
            buf << " \u0001#{name}:#{Zlib.crc32(squash(c.to_html))}\u0002 "
          elsif name == 'a'
            buf << "\u0001a:#{c['href']}"
            sig(c, buf)
            buf << "\u0002"
          elsif MARKUP.include?(name)
            buf << "\u0001#{name}"
            sig(c, buf)
            buf << "\u0002"
          elsif name == 'br'
            buf << ' '
          elsif %w[title].include?(cls.split.first) || name == 'caption'
            inner = +''
            sig(c, inner)
            buf << ' ' << squash(inner).sub(CAPTION, '') << ' '
          else
            buf << ' '
            sig(c, buf)
            buf << ' '
          end
        end
      end
    end

    def words(text)
      text.delete("\u0001\u0002").scan(/[[:alnum:]_]+/)
    end

    # How alike two lists of words are: the share of words in common, 0..1
    def similarity(a, b)
      return 1.0 if a.empty? && b.empty?
      return 0.0 if a.empty? || b.empty?

      counts = Hash.new(0)
      a.each { |w| counts[w] += 1 }
      common = b.count { |w| counts[w].positive? && (counts[w] -= 1) >= 0 }
      2.0 * common / (a.size + b.size)
    end

    # -------------------------------------------------------------------------------- Alignment

    # The pairs `[i, j]` of a longest common subsequence of `a` and `b`, in order; `weight` gives
    # the worth of matching an element (1 each by default)
    def lcs(a, b, weight = nil)
      pre = 0
      pre += 1 while pre < a.size && pre < b.size && a[pre] == b[pre]
      suf = 0
      suf += 1 while suf < a.size - pre && suf < b.size - pre && a[a.size - 1 - suf] == b[b.size - 1 - suf]
      middle = lcs_middle(a[pre...(a.size - suf)], b[pre...(b.size - suf)], weight)
      (0...pre).map { |i| [i, i] } +
        middle.map { |i, j| [i + pre, j + pre] } +
        (0...suf).map { |k| [a.size - suf + k, b.size - suf + k] }
    end

    def lcs_middle(a, b, weight)
      return [] if a.empty? || b.empty?
      return lcs_greedy(a, b) if a.size * b.size > 4_000_000

      n = a.size
      m = b.size
      len = Array.new(n + 1) { Array.new(m + 1, 0) }
      (n - 1).downto(0) do |i|
        row = len[i]
        below = len[i + 1]
        ai = a[i]
        (m - 1).downto(0) do |j|
          row[j] = ai == b[j] ? below[j + 1] + (weight ? weight.call(ai) : 1) : [below[j], row[j + 1]].max
        end
      end
      pairs = []
      i = j = 0
      while i < n && j < m
        if a[i] == b[j]
          pairs << [i, j]
          i += 1
          j += 1
        elsif len[i + 1][j] >= len[i][j + 1]
          i += 1
        else
          j += 1
        end
      end
      pairs
    end

    # For very long sequences: each element of `a`, in order, to its next occurrence in `b`
    def lcs_greedy(a, b)
      where = Hash.new { |h, k| h[k] = [] }
      b.each_with_index { |x, j| where[x] << j }
      last = -1
      a.each_with_index.filter_map do |x, i|
        j = where[x].bsearch { |k| k > last } or next
        last = j
        [i, j]
      end
    end

    # Sets the status and the peer of every block of both pages
    def align(old_blocks, new_blocks)
      ident = ->(b) { "#{b.kind}\0#{b.key}" }
      pairs = lcs(old_blocks.map(&ident), new_blocks.map(&ident))
      oi = nj = 0
      (pairs + [[old_blocks.size, new_blocks.size]]).each do |i, j|
        pair_gap(old_blocks[oi...i], new_blocks[nj...j])
        if i < old_blocks.size
          old_blocks[i].status = new_blocks[j].status = :same
          old_blocks[i].peer = new_blocks[j]
          new_blocks[j].peer = old_blocks[i]
        end
        oi = i + 1
        nj = j + 1
      end
    end

    # Between two aligned blocks: an inserted block that is like a removed one changed it
    def pair_gap(gone, added)
      from = 0
      added.each do |b|
        k = (from...gone.size).find { |x| like?(gone[x], b) }
        if k
          gone[from...k].each { |g| g.status = :deleted }
          gone[k].status = b.status = :changed
          gone[k].peer = b
          b.peer = gone[k]
          from = k + 1
        else
          b.status = :added
        end
      end
      gone[from..].each { |g| g.status = :deleted }
    end

    def like?(old_block, new_block)
      return false unless old_block.kind == new_block.kind

      WHOLE.include?(new_block.kind) || similarity(old_block.words, new_block.words) >= SIMILAR
    end

    # ---------------------------------------------------------------------------------- Marking

    def mark_new_page(doc)
      title = doc.at_css('#header > h1') or return
      title.add_child(badge(doc, 'new', 'New page'))
    end

    def badge(doc, kind, label)
      span = Nokogiri::XML::Node.new('span', doc)
      span['class'] = "nx-diff-badge nx-diff-badge-#{kind}"
      span.content = label
      span
    end

    def mark_changes(doc, old_doc, stats)
      old_blocks = blocks(old_doc)
      new_blocks = blocks(doc)
      align(old_blocks, new_blocks)
      changed = []

      units(new_blocks, :added).each do |el|
        tag(el, 'added')
        heading_badge(doc, el, 'new', 'New')
        stats[:added] += 1
        changed << el
      end
      new_blocks.each do |b|
        next unless b.status == :changed

        tag(target(b), 'changed')
        inline(b.peer, b)
        stats[:changed] += 1
        changed << target(b)
      end
      ghosts(doc, old_blocks).each do |ghost|
        stats[:deleted] += 1
        changed << ghost
      end
      flag_toc(doc, changed)
      count_runs(doc, stats)
    end

    def tag(el, kind)
      el['class'] = [el['class'], 'nx-diff', "nx-diff-#{kind}"].compact.join(' ')
      el['data-nx-change'] = kind
    end

    def heading_badge(doc, el, kind, label)
      return unless first_class(el).match?(/\Asect\d\z/)

      heading = el.element_children.find { |c| HEADINGS.include?(c.name) } or return
      heading.add_child(badge(doc, kind, label))
    end

    # The topmost elements standing for the blocks of one status: a container whose blocks all
    # have it, else the block's element when nothing else is in it, else its mark
    def units(page_blocks, status)
      total = Hash.new(0)
      hits = Hash.new(0)
      chain = {}
      page_blocks.each do |b|
        chain[b] = b.el.ancestors.select { |a| a.element? && unit?(a) }.reverse
        (chain[b] + [b.el]).uniq.each do |c|
          total[c] += 1
          hits[c] += 1 if b.status == status
        end
      end
      els = page_blocks.filter_map do |b|
        next unless b.status == status

        chain[b].find { |c| hits[c] == total[c] } || (hits[b.el] == total[b.el] ? b.el : b.mark)
      end
      prune(els.uniq)
    end

    # The element to mark for a changed block: its own element, unless that holds other blocks
    def target(block)
      return block.el if block.el == block.mark

      alone = block.el.element_children.all? { |c| c == block.mark || first_class(c) == 'title' }
      alone ? block.el : block.mark
    end

    # Drops the elements inside another of the list
    def prune(els)
      set = els.to_h { |e| [e, true] }
      els.reject { |e| e.ancestors.any? { |a| set[a] } }
    end

    def unit?(el)
      return true if el.name == 'table' || el.name == 'li' || el.name == 'dd'

      el.name == 'div' && UNITS.include?(first_class(el))
    end

    # ---------------------------------------------------------------------------------- Ghosts

    # Puts a struck copy of each block (or all-gone container) of the old page where it stood;
    # returns the copies
    def ghosts(doc, old_blocks)
      peer_of = {}
      old_blocks.each { |b| peer_of[b.el] = b.peer.el if b.peer }
      # Marks the old elements that stay, to cut them out of a copied container
      old_blocks.each { |b| b.el['data-nx-keep'] = '' if b.peer }
      placed = {}
      units(old_blocks, :deleted).map do |el|
        ghost = el.dup(1)
        ghost.css('[data-nx-keep]').each(&:remove)
        ghost.xpath('descendant-or-self::*[@id]').each { |e| e.remove_attribute('id') }
        tag(ghost, 'deleted')
        heading_badge(doc, ghost, 'deleted', 'Deleted')
        place(doc, el, ghost, peer_of, placed)
        placed[el] = ghost
        ghost
      end
    end

    # The element of the new page for element `el` of the old one: its copy if it is gone, its
    # peer, or found through the last block inside it that has a peer, as many levels up from it
    def counterpart(el, peer_of, placed)
      return placed[el] if placed[el]
      return peer_of[el] if peer_of[el]

      inner = el.css('*').reverse.find { |d| peer_of[d] } or return nil
      found = peer_of[inner]
      depth = inner.ancestors.index(el) or return nil
      (depth + 1).times { found = found.parent or return nil }
      found if found.element? && found.name == el.name && first_class(found) == first_class(el)
    end

    # Puts `ghost` beside the nearest sibling of `el` that has a counterpart in the new page,
    # looking back first, then forward, then into the parent's counterpart, then up
    def place(doc, el, ghost, peer_of, placed)
      at = el
      while at.parent&.element?
        sib = at.previous_element
        while sib
          c = counterpart(sib, peer_of, placed) and return c.add_next_sibling(ghost)
          sib = sib.previous_element
        end
        sib = at.next_element
        while sib
          c = counterpart(sib, peer_of, placed) and return c.add_previous_sibling(ghost)
          sib = sib.next_element
        end
        at = at.parent
        c = counterpart(at, peer_of, placed)
        # No sibling stayed: the parent's first place, so a removal reads before what replaced it
        return c.children.empty? ? c.add_child(ghost) : c.children.first.add_previous_sibling(ghost) if c
      end
      holder = doc.at_css('#content') || doc.at_css('body')
      holder.children.empty? ? holder.add_child(ghost) : holder.children.first.add_previous_sibling(ghost)
    end

    # ---------------------------------------------------------------------------------- Inline

    # The words that changed, in a changed block: inserted ones highlighted, removed ones struck
    def inline(old_block, new_block)
      case new_block.kind
      when 'row'
        old_cells = old_block.mark.xpath('./td | ./th')
        new_cells = new_block.mark.xpath('./td | ./th')
        return unless old_cells.size == new_cells.size

        old_cells.zip(new_cells).each { |o, n| inline_html(o, n) unless squash(o.inner_html) == squash(n.inner_html) }
      when 'paragraph', 'item', 'description', 'term', 'heading', 'title'
        inline_html(old_block.mark, new_block.mark)
      when 'admonition'
        o = old_block.mark.at_css('td.content')
        n = new_block.mark.at_css('td.content')
        inline_html(o, n) if o && n && o.css('div').empty? && n.css('div').empty?
      end
    end

    def inline_html(old_el, new_el)
      new_el.inner_html = word_diff(old_el.inner_html, new_el.inner_html)
    end

    # A word, a tag, an entity or another character, with the white space after it
    TOKEN = /\A\s+|(?:<[^>]*>|&[#[:alnum:]]+;|[[:alnum:]_]+|[^<\s])\s*/

    # The markup of `new_html` with `<ins>` around its new words and `<del>` with the removed words
    # of `old_html` (their markup dropped, so the tags are always those of the new version). Words
    # weigh by their length, so the alignment holds on to words rather than to commas and tags.
    def word_diff(old_html, new_html)
      old_tokens = old_html.scan(TOKEN)
      new_tokens = new_html.scan(TOKEN)
      weight = ->(t) { t.start_with?('<') || t.size < 2 ? 1 : [t.size, 12].min }
      pairs = lcs(old_tokens.map(&:strip), new_tokens.map(&:strip), weight)
      ops = []
      oi = nj = 0
      (pairs + [[old_tokens.size, new_tokens.size]]).each do |i, j|
        ops << [:change, old_tokens[oi...i], new_tokens[nj...j]] if i > oi || j > nj
        ops << [:same, [], [new_tokens[j]]] if j < new_tokens.size && i < old_tokens.size
        oi = i + 1
        nj = j + 1
      end
      render_ops(merge_ops(ops))
    end

    # Joins runs of equal tokens, then folds a lone short word between two changes into them, so a
    # rewritten phrase reads as one deletion and one insertion
    def merge_ops(ops)
      merged = []
      ops.each_with_index do |op, k|
        glue = op[0] == :same && merged.last&.first == :change && ops[k + 1]&.first == :change &&
               op[2].size == 1 && !op[2][0].start_with?('<') && op[2][0].strip.size <= 3
        if glue
          merged.last[1] += op[2]
          merged.last[2] += op[2]
        elsif merged.last && merged.last[0] == op[0]
          merged.last[1] += op[1]
          merged.last[2] += op[2]
        else
          merged << [op[0], op[1].dup, op[2].dup]
        end
      end
      merged
    end

    def render_ops(ops)
      out = +''
      ops.each do |kind, old_tokens, new_tokens|
        if kind == :same
          out << new_tokens.join
          next
        end
        gone = old_tokens.map { |t| t.start_with?('<') ? t[/\s*\z/] : t }.join
        out << %(<del class="nx-del">#{gone.rstrip}</del>#{gone[/\s*\z/]}) unless gone.strip.empty?
        run = +''
        new_tokens.each do |t|
          if t.start_with?('<')
            out << wrap_ins(run) << t
            run = +''
          else
            run << t
          end
        end
        out << wrap_ins(run)
      end
      out
    end

    def wrap_ins(text)
      return text if text.strip.empty?

      lead = text[/\A\s*/]
      trail = text[/\s*\z/]
      %(#{lead}<ins class="nx-ins">#{text.strip}</ins>#{trail})
    end

    # --------------------------------------------------------------------------------- Chrome

    # Flags the entries of the table of contents whose sections hold a change
    def flag_toc(doc, changed)
      ids = {}
      changed.each do |el|
        [el, *el.ancestors].each do |a|
          next unless a.element? && first_class(a).match?(/\Asect\d\z/)

          heading = a.element_children.find { |c| HEADINGS.include?(c.name) }
          next unless heading && heading['id']

          ids[heading['id']] = a == el && el['data-nx-change'] == 'added' ? 'nx-diff-toc nx-diff-toc-new' : 'nx-diff-toc'
        end
      end
      doc.css('#toc a').each do |a|
        flag = ids[a['href'].to_s.delete_prefix('#')] or next
        a['class'] = [a['class'], flag].compact.join(' ')
      end
    end

    # Makes a run of marks of one kind, next to each other (rows, items, paragraphs), one change
    # to count and to jump to; the counts are those of the changes
    def count_runs(doc, stats)
      %i[added changed deleted].each { |k| stats[k] = 0 }
      doc.css('[data-nx-change]').each do |el|
        kind = el['data-nx-change']
        before = el.previous_element
        if before && before['data-nx-change'].nil? && before['data-nx-run'] == kind || before&.[]('data-nx-change') == kind
          el.remove_attribute('data-nx-change')
          el['data-nx-run'] = kind
        else
          stats[kind.to_sym] += 1
        end
      end
    end

    # Flags the links of the book's navigation to the pages that changed
    def flag_pages(doc, pages)
      return if pages.empty?

      doc.css('.nx-pager a, .nx-crumbs a').each do |a|
        name = a['href'].to_s.delete_suffix('.html')
        a['class'] = [a['class'], 'nx-diff-page'].compact.join(' ') if pages.include?(name)
      end
    end

    def finish(doc, stats, ref, head, body, pages)
      html = doc.at_css('body')
      html['data-nx-diff-ref'] = ref
      stats.each { |k, v| html["data-nx-diff-#{k.to_s.tr('_', '-')}"] = v.to_s }
      html['data-nx-diff-pages'] = pages.join(' ') unless pages.empty?
      doc.at_css('head')&.add_child(head) unless head.empty?
      doc.at_css('body')&.add_child(body) unless body.empty?
    end
  end
end
