# frozen_string_literal: true

# Part of Nexus Library
#
# Plugin: the theme of the pages, in <head> after the default stylesheet of Asciidoctor: the tokens
# (`res/adoc/tokens.css`: color seeds, fonts, spacing), the colors derived from them
# (`res/adoc/palette.css`), then the style levels up to the one chosen.
# The levels build on each other:
#
#   plain     colors, fonts and a tight rhythm; rules instead of boxes
#   compact   + dark code blocks, framed tables, admonition bars, ToC markers
#   full      + icons, legends, notes, key points, figure cards (the default)
#
# Chosen by the document attribute `:theme-style:` or `$ADOC_STYLE`.
#
# The base size, `:theme-size:` or `$ADOC_SIZE` (small 15px, normal 16px, large
# 17px), overrides `--text` of the tokens; everything is in rem, so the whole
# page scales.
#
# `$ADOC_TOKENS` replaces the tokens (e.g. a file exported from the
# Color or Font Studio).
#
# `$ADOC_CSS` replaces the whole theme with a stylesheet of the project.
#
# The icons (`Nxs::Icons`, `lib/icons.rb`) come first, with every theme: `--icon-<name>` and the
# rules of `i:<name>[]`.

require 'asciidoctor'
require 'asciidoctor/extensions'
require_relative '../lib/icons'

module Nxs
  class Theme < Asciidoctor::Extensions::DocinfoProcessor
    use_dsl
    at_location :head

    DIR = File.expand_path('../../../res/adoc', __dir__)
    STYLES = %w[plain compact full].freeze
    SIZES = { 'small' => 15, 'normal' => 16, 'large' => 17 }.freeze # as `SIZES` of studio/studio.js

    def process(document)
      icons = "<style>\n#{Icons.css}</style>"
      return [icons, style(ENV.fetch('ADOC_CSS'))].join("\n") if ENV.key?('ADOC_CSS')

      name = document.attr('theme-style', ENV.fetch('ADOC_STYLE', 'full'))
      level = STYLES.index(name) or
        raise ArgumentError, "theme: unknown style `#{name}` (styles: #{STYLES.join(', ')})"

      tokens = ENV.fetch('ADOC_TOKENS', File.join(DIR, 'tokens.css'))
      layers = STYLES[0..level].map { |s| File.join(DIR, 'styles', "#{s}.css") }
      palette = File.join(DIR, 'palette.css')
      [icons, *([tokens, palette] + layers).map { |f| style(f) }, size(document)].compact.join("\n")
    end

    private

    def size(document)
      name = document.attr('theme-size', ENV.fetch('ADOC_SIZE', nil)) or return
      px = SIZES[name] or raise ArgumentError, "theme: unknown size `#{name}` (sizes: #{SIZES.keys.join(', ')})"
      "<style>:root { --text: #{px}px; }</style>"
    end

    def style(file)
      "<style>\n#{File.read(file)}</style>"
    end
  end
end

Asciidoctor::Extensions.register do
  docinfo_processor Nxs::Theme
end
