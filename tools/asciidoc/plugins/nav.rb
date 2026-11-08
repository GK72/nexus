# frozen_string_literal: true

# Part of Nexus Library
#
# Plugin: the navigation of a book's pages. When `tools/doc.sh` renders a page of a book, it sets
# `nxs-book` to the spine; the page then gets a header bar of breadcrumbs (the book, linking to its
# `index.html`, and the page) and a footer bar of the previous and next pages, in reading order: the
# includes of `pages/` from the spine, depth first. Both bars stay in view while scrolling. The spine
# itself and single documents get none.

require 'asciidoctor'
require 'asciidoctor/extensions'
require 'cgi'

module Nxs
  module Nav
    # The book of a page: its title and its pages in reading order, `[[name, title], ...]`: the
    # includes of the spine, depth first, so a page that includes pages (a part) comes before them
    def self.book(spine)
      pages_dir = File.join(File.dirname(spine), 'pages')
      pages = []
      walk = lambda do |file|
        File.foreach(file) do |line|
          m = line.match(/\Ainclude::([^\[]+\.adoc)\[/) or next
          page = File.expand_path(m[1], File.dirname(file))
          name = File.basename(page, '.adoc')
          next unless File.dirname(page) == pages_dir && File.exist?(page) && !name.start_with?('_')
          next if pages.any? { |n, _| n == name }

          pages << [name, title(page) || name]
          walk.call(page)
        end
      end
      walk.call(spine)
      { title: title(spine) || File.basename(spine, '.adoc'), pages: pages }
    end

    def self.title(file)
      File.foreach(file) { |line| return line[2..].strip if line.start_with?('= ') }
      nil
    end
  end

  # The styles of the navigation, with fallbacks for a project stylesheet without the tokens
  class NavStyle < Asciidoctor::Extensions::DocinfoProcessor
    use_dsl
    at_location :head

    CSS = <<~CSS
      <style>
      .nx-crumbs, .nx-pager { position: sticky; z-index: 5; background: var(--bg, #fff);
        font-size: 0.85rem; color: var(--muted, #6b7280); }
      .nx-crumbs { top: 0; border-bottom: 1px solid var(--rule, #d1d5db); }
      .nx-pager { bottom: 0; border-top: 1px solid var(--rule, #d1d5db); }
      .nx-bar { display: flex; align-items: center; gap: var(--gap, 0.6rem); max-width: var(--measure, 62.5rem);
        margin: 0 auto; padding: calc(var(--gap, 0.6rem) * 0.6) 1rem; }
      .nx-crumbs a, .nx-pager a { color: inherit; text-decoration: none; }
      .nx-crumbs a:hover, .nx-pager a:hover { color: var(--link, #2563eb); }
      .nx-crumbs a::after { content: "\u203a"; margin: 0 0.45em; color: var(--rule, #d1d5db); }
      .nx-pager a { color: var(--link, #2563eb); }
      .nx-prev::before { content: "\u2190 "; }
      .nx-next { margin-left: auto; }
      .nx-next::after { content: " \u2192"; }
      html { scroll-padding-top: 3rem; scroll-padding-bottom: 3rem; }
      @media print { .nx-crumbs, .nx-pager { position: static; } }
      </style>
    CSS

    def process(document)
      CSS if document.attr?('nxs-book')
    end
  end

  # Breadcrumbs, before the header of the page
  class NavDocinfo < Asciidoctor::Extensions::DocinfoProcessor
    use_dsl
    at_location :header

    def process(document)
      spine = document.attr('nxs-book') or return
      book = Nav.book(spine)
      here = File.basename(document.attr('docfile'), '.adoc')
      page = book[:pages].find { |name, _| name == here }&.last || document.doctitle.to_s
      %(<nav class="nx-crumbs" aria-label="Breadcrumbs"><div class="nx-bar">) +
        %(<a href="index.html">#{CGI.escapeHTML(book[:title])}</a>) +
        %(<span aria-current="page">#{CGI.escapeHTML(page)}</span></div></nav>)
    end
  end

  # Previous and next, a footer bar at the end of the body
  class NavPager < Asciidoctor::Extensions::DocinfoProcessor
    use_dsl
    at_location :footer

    def process(document)
      spine = document.attr('nxs-book') or return
      pages = Nav.book(spine)[:pages]
      index = pages.index { |name, _| name == File.basename(document.attr('docfile'), '.adoc') } or return
      link = ->((name, title), cls) { %(<a class="#{cls}" href="#{name}.html">#{CGI.escapeHTML(title)}</a>) }
      prev = index.positive? ? link[pages[index - 1], 'nx-prev'] : ''
      nxt = pages[index + 1] ? link[pages[index + 1], 'nx-next'] : ''
      %(<nav class="nx-pager" aria-label="Pages"><div class="nx-bar">#{prev}#{nxt}</div></nav>)
    end
  end
end

Asciidoctor::Extensions.register do
  docinfo_processor Nxs::NavStyle
  docinfo_processor Nxs::NavDocinfo
  docinfo_processor Nxs::NavPager
end
