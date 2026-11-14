# frozen_string_literal: true

# Part of Nexus Library
#
# Plugin: the changes of a page since a commit, for `tools/doc.sh --diff[=REF]`. The script renders
# the page at REF first and sets
#
#   nxs-diff         the HTML of the page at REF, or `-` when the page is new
#   nxs-diff-ref     the name of REF, for the page's toolbar
#   nxs-diff-pages   the pages of the book that changed, by name, separated by spaces
#
# The rendered page then gets its changes marked (`Nxs::Diff`, `lib/diff.rb`), with the styles
# (`diff/diff.css`) and the toolbar (`diff/diff.js`): marks and deletions on or off, the previous
# and next change. Pages without a change, and pages rendered without `--diff`, carry nothing.

require 'asciidoctor'
require 'asciidoctor/extensions'

module Nxs
  class DiffMarks < Asciidoctor::Extensions::Postprocessor
    use_dsl

    DIR = File.join(__dir__, 'diff')

    def process(document, output)
      ref = document.attr('nxs-diff') or return output
      require_relative '../lib/diff'

      old = ref == '-' ? nil : File.read(ref)
      pages = document.attr('nxs-diff-pages', '').split
      html, = Diff.apply(output, old, ref: document.attr('nxs-diff-ref', 'HEAD'), pages: pages,
                                      head: "<style>\n#{File.read(File.join(DIR, 'diff.css'))}</style>",
                                      body: "<script>\n#{File.read(File.join(DIR, 'diff.js'))}</script>")
      html
    end
  end
end

Asciidoctor::Extensions.register do
  postprocessor Nxs::DiffMarks
end
