# frozen_string_literal: true

# Part of Nexus Library
#
# Unit tests of `Nxs::Diff` (`diff.rb`), in memory: pages as strings, built like the HTML of
# Asciidoctor.
#
#   ruby tools/asciidoc/lib/diff_test.rb

ENV['MT_NO_PLUGINS'] = '1' # minitest only, no plugins of other gems
require 'minitest/autorun'
require_relative 'diff'

class DiffTest < Minitest::Test
  # A page of Asciidoctor's shape: a title, a table of contents of the sections, the content
  def page(content, title: 'Page')
    toc = content.scan(/<h2 id="([^"]+)">([^<]*)/).map { |id, text| %(<li><a href="##{id}">#{text}</a></li>) }
    <<~HTML
      <!DOCTYPE html>
      <html lang="en">
      <head><title>#{title}</title></head>
      <body class="article">
      <div id="header"><h1>#{title}</h1><div id="toc" class="toc2"><ul class="sectlevel1">#{toc.join}</ul></div></div>
      <div id="content">
      #{content}
      </div>
      </body>
      </html>
    HTML
  end

  def para(text)
    %(<div class="paragraph"><p>#{text}</p></div>)
  end

  def section(id, title, *blocks)
    %(<div class="sect1"><h2 id="#{id}">#{title}</h2><div class="sectionbody">#{blocks.join}</div></div>)
  end

  def table(*rows, caption: nil)
    cap = caption ? %(<caption class="title">#{caption}</caption>) : ''
    body = rows.map { |cells| "<tr>#{cells.map { |c| %(<td class="tableblock"><p class="tableblock">#{c}</p></td>) }.join}</tr>" }
    %(<table class="tableblock frame-all">#{cap}<tbody>#{body.join}</tbody></table>)
  end

  def list(*items)
    %(<div class="ulist"><ul>#{items.map { |i| "<li><p>#{i}</p></li>" }.join}</ul></div>)
  end

  def figure(svg, title)
    %(<div class="imageblock figure"><div class="content">#{svg}</div><div class="title">#{title}</div></div>)
  end

  def compare(old_content, new_content, **opts)
    old_html = old_content && page(old_content)
    html, stats = Nxs::Diff.apply(page(new_content), old_html, **opts)
    [Nokogiri::HTML5(html), stats, html]
  end

  def counts(stats)
    stats.values_at(:added, :changed, :deleted)
  end

  # ------------------------------------------------------------------------------- No change

  def test_the_same_page_comes_back_as_it_is
    content = section('_a', 'A', para('One two three.'), list('x', 'y'))
    html = page(content)
    out, stats = Nxs::Diff.apply(html, page(content))
    assert_same html, out
    assert_equal [0, 0, 0], counts(stats)
  end

  def test_rewrapped_source_is_no_change
    _, stats, = compare(para("One two\nthree four."), para("One\ntwo three   four."))
    assert_equal [0, 0, 0], counts(stats)
  end

  def test_shifted_caption_numbers_are_no_change
    old = section('_a', 'A', table(%w[a b], caption: 'Table 3. Costs.'))
    new = section('_a', 'A', table(%w[a b], caption: 'Table 4. Costs.'))
    _, stats, = compare(old, new)
    assert_equal [0, 0, 0], counts(stats)
  end

  def test_a_change_of_markup_is_a_change
    _, stats, = compare(para('Use the tool.'), para('Use the <code>tool</code>.'))
    assert_equal [0, 1, 0], counts(stats)
  end

  # ---------------------------------------------------------------------------------- Blocks

  def test_a_new_paragraph_is_added
    doc, stats, = compare(para('First.') + para('Last.'), para('First.') + para('A new one in between.') + para('Last.'))
    assert_equal [1, 0, 0], counts(stats)
    marked = doc.at_css('.nx-diff-added')
    assert_equal 'A new one in between.', marked.text
    assert_equal 'added', marked['data-nx-change']
  end

  def test_a_changed_paragraph_shows_its_words
    doc, stats, = compare(para('The edge runs the reducer shards.'), para('The edge runs the outbox.'))
    assert_equal [0, 1, 0], counts(stats)
    assert doc.at_css('div.paragraph.nx-diff-changed')
    assert_equal 'reducer shards', doc.at_css('del.nx-del').text
    assert_equal 'outbox', doc.at_css('ins.nx-ins').text
  end

  def test_a_removed_paragraph_stays_in_place_struck
    doc, stats, = compare(para('First.') + para('Gone now.') + para('Last.'), para('First.') + para('Last.'))
    assert_equal [0, 0, 1], counts(stats)
    texts = doc.css('#content div.paragraph').map(&:text)
    assert_equal ['First.', 'Gone now.', 'Last.'], texts
    assert_includes doc.css('#content div.paragraph')[1]['class'], 'nx-diff-deleted'
  end

  def test_a_rewritten_paragraph_is_removed_and_added
    doc, stats, = compare(para('Aggregations are code in the image.'), para('Each question is a release of its own.'))
    assert_equal [1, 0, 1], counts(stats)
    assert_equal ['Aggregations are code in the image.', 'Each question is a release of its own.'],
                 doc.css('#content div.paragraph').map(&:text)
  end

  def test_a_removed_block_loses_its_ids
    doc, = compare(section('_a', 'A', para('Keep.')) + section('_b', 'B', para('Old.')), section('_a', 'A', para('Keep.')))
    ghost = doc.at_css('.nx-diff-deleted')
    assert_equal 'div', ghost.name
    assert_empty ghost.css('[id]')
    assert_equal 1, doc.css('#_a').size
  end

  def test_a_figure_that_changed_is_marked_whole
    old = figure('<svg><rect width="10"/></svg>', 'Figure 1. A box.')
    new = figure('<svg><rect width="20"/></svg>', 'Figure 1. A box.')
    doc, stats, = compare(old, new)
    assert_equal [0, 1, 0], counts(stats)
    assert doc.at_css('div.imageblock.nx-diff-changed')
    assert_empty doc.css('ins, del')
  end

  # ----------------------------------------------------------------------- Rows and items

  def test_rows_added_together_are_one_change
    old = table(%w[a 1], %w[d 4])
    new = table(%w[a 1], %w[b 2], %w[c 3], %w[d 4])
    doc, stats, = compare(old, new)
    assert_equal [1, 0, 0], counts(stats)
    assert_equal 2, doc.css('tr.nx-diff-added').size
    assert_equal 1, doc.css('tr[data-nx-change]').size
  end

  def test_a_changed_row_shows_the_words_of_its_cells
    old = table(['edge', 'ingest, reducer shards, the outbox'])
    new = table(['edge', 'ingest, the outbox'])
    doc, stats, = compare(old, new)
    assert_equal [0, 1, 0], counts(stats)
    assert doc.at_css('tr.nx-diff-changed')
    assert_equal 'reducer shards,', doc.at_css('td del.nx-del').text
  end

  def test_a_removed_row_stays_in_its_table
    doc, stats, = compare(table(%w[a 1], %w[b 2], %w[c 3]), table(%w[a 1], %w[c 3]))
    assert_equal [0, 0, 1], counts(stats)
    assert_equal %w[a1 b2 c3], doc.css('tbody tr').map(&:text)
    assert_equal 'tr', doc.at_css('.nx-diff-deleted').name
  end

  def test_a_removed_item_stays_in_its_list
    doc, stats, = compare(list('one', 'two', 'three'), list('one', 'three'))
    assert_equal [0, 0, 1], counts(stats)
    assert_equal %w[one two three], doc.css('li').map(&:text)
    assert_includes doc.css('li')[1]['class'], 'nx-diff-deleted'
  end

  def test_a_new_list_is_one_change
    doc, stats, = compare(para('Intro.'), para('Intro.') + list('a', 'b', 'c'))
    assert_equal [1, 0, 0], counts(stats)
    assert doc.at_css('div.ulist.nx-diff-added')
    assert_empty doc.css('li.nx-diff')
  end

  # --------------------------------------------------------------------- Sections and pages

  def test_a_new_section_is_marked_once_with_a_badge_and_in_the_toc
    old = section('_a', 'A', para('Kept.'))
    new = old + section('_b', 'B', para('New text.'), list('x'))
    doc, stats, = compare(old, new)
    assert_equal [1, 0, 0], counts(stats)
    sect = doc.at_css('div.sect1.nx-diff-added')
    assert_equal 'B', sect.at_css('h2').children.first.text
    assert_equal 'New', sect.at_css('h2 .nx-diff-badge-new').text
    assert_empty sect.css('.nx-diff')
    assert_includes doc.at_css('#toc a[href="#_b"]')['class'], 'nx-diff-toc-new'
    assert_nil doc.at_css('#toc a[href="#_a"]')['class']
  end

  def test_the_toc_flags_the_section_of_a_change
    old = section('_a', 'A', para('Kept.')) + section('_b', 'B', para('Old words here.'))
    new = section('_a', 'A', para('Kept.')) + section('_b', 'B', para('New words here.'))
    doc, = compare(old, new)
    assert_equal 'nx-diff-toc', doc.at_css('#toc a[href="#_b"]')['class']
  end

  def test_a_new_page_gets_a_badge_and_no_marks
    doc, stats, = compare(nil, section('_a', 'A', para('All new.')))
    assert stats[:new_page]
    assert_equal 'New page', doc.at_css('#header h1 .nx-diff-badge-new').text
    assert_empty doc.css('[data-nx-change]')
  end

  def test_the_assets_and_the_counts_go_into_a_page_with_changes
    doc, = compare(para('a b c d.'), para('a b c e.'), ref: 'v1', head: '<style>/*x*/</style>',
                                                    body: '<script>/*y*/</script>')
    body = doc.at_css('body')
    assert_equal 'v1', body['data-nx-diff-ref']
    assert_equal '1', body['data-nx-diff-changed']
    assert_includes doc.at_css('head style').text, '/*x*/'
    assert_includes doc.css('body script').last.text, '/*y*/'
  end

  def test_the_navigation_flags_the_pages_that_changed
    nav = '<nav class="nx-pager"><a class="nx-prev" href="a.html">A</a><a class="nx-next" href="b.html">B</a></nav>'
    html, = Nxs::Diff.apply(page(para('x')).sub('</body>', "#{nav}</body>"), page(para('x')), pages: %w[b])
    doc = Nokogiri::HTML5(html)
    assert_equal 'nx-prev', doc.at_css('a[href="a.html"]')['class']
    assert_equal 'nx-next nx-diff-page', doc.at_css('a[href="b.html"]')['class']
  end

  # ----------------------------------------------------------------------------- Algorithms

  def test_lcs_pairs_the_common_elements_in_order
    assert_equal [[0, 0], [2, 1], [3, 3]], Nxs::Diff.lcs(%w[a b c d], %w[a c x d])
    assert_empty Nxs::Diff.lcs(%w[a], %w[b])
  end

  def test_word_diff_keeps_the_new_markup
    out = Nxs::Diff.word_diff('Run <code>old</code> now.', 'Run <code>new</code> now.')
    assert_equal 'Run <code><del class="nx-del">old</del><ins class="nx-ins">new</ins></code> now.', out
  end

  def test_word_diff_holds_on_to_words_rather_than_commas
    out = Nxs::Diff.word_diff('a StatefulSet of shards, the ingest Service, the client certificate',
                              'the ingest Service, the outbox, the client certificate')
    assert_equal '<del class="nx-del">a StatefulSet of shards,</del> the ingest Service<ins class="nx-ins">, the outbox</ins>, ' \
                 'the client certificate', out
  end

  def test_similarity_is_the_share_of_common_words
    assert_in_delta 1.0, Nxs::Diff.similarity(%w[a b], %w[b a])
    assert_in_delta 0.5, Nxs::Diff.similarity(%w[a b], %w[a c])
    assert_in_delta 0.0, Nxs::Diff.similarity(%w[a], [])
  end
end
