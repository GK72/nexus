# frozen_string_literal: true

# Part of Nexus Library
#
# The theme of the pages: the stylesheet `$ADOC_CSS` (default: `res/adoc/theme.css`) in <head>,
# after the default stylesheet of Asciidoctor. The stylesheet brings its own fonts (`@import`).

require 'asciidoctor'
require 'asciidoctor/extensions'

module Adoc
  class Theme < Asciidoctor::Extensions::DocinfoProcessor
    use_dsl
    at_location :head

    CSS = ENV.fetch('ADOC_CSS', File.expand_path('../../res/adoc/theme.css', __dir__))

    def process(_document)
      "<style>\n#{File.read(CSS)}</style>"
    end
  end
end

Asciidoctor::Extensions.register do
  docinfo_processor Adoc::Theme
end
