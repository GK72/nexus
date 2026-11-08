# frozen_string_literal: true

# Part of Nexus Library
#
# Plugin: `studio::color[]` and `studio::font[]`, the Color and Font Studio: tools in the page that
# edit the theme tokens of the page itself.
#
#   studio::color[]    the color seeds (OKLCH) of the tokens, with the contrast of every text color
#   studio::font[]     families, size, leading, scale and spacing
#
# The studios read the tokens of the theme (`res/adoc/tokens.css`), show the edits on the whole page
# and export a tokens file. Pages with a studio get the styles
# (`studio/studio.css`) and the scripts: `studio/studio.js`, then `color.js` and/or `font.js`.

require 'asciidoctor'
require 'asciidoctor/extensions'

module Nxs
  module Studio
    DIR = File.join(__dir__, 'studio')
    KINDS = %w[color font].freeze

    # The kinds a page uses, kept on the document: not in its attributes, which Asciidoctor resets
    # before the conversion, where the docinfo is written
    def self.used(document)
      document.instance_variable_get(:@nx_studios) || document.instance_variable_set(:@nx_studios, [])
    end
  end

  # `studio::<kind>[]`
  class StudioMacro < Asciidoctor::Extensions::BlockMacroProcessor
    use_dsl
    named :studio

    def process(parent, target, _attrs)
      Studio::KINDS.include?(target) or
        raise ArgumentError, "studio: unknown studio `#{target}` (studios: #{Studio::KINDS.join(', ')})"

      kinds = Studio.used(parent.document)
      kinds << target unless kinds.include?(target)
      html = %(<div class="studio" data-studio="#{target}">) +
             %(<noscript>The #{target} studio needs JavaScript.</noscript></div>)
      create_pass_block(parent, html, {}, subs: nil)
    end
  end

  # The styles in <head>; the core and the studios used, at the end of <body>.
  class StudioAssets < Asciidoctor::Extensions::DocinfoProcessor
    use_dsl

    def process(document)
      kinds = Studio.used(document)
      return if kinds.empty?

      if config[:location] == :head
        "<style>\n#{File.read(File.join(Studio::DIR, 'studio.css'))}</style>"
      else
        (['studio'] + kinds).map { |f| "<script>\n#{File.read(File.join(Studio::DIR, "#{f}.js"))}</script>" }.join("\n")
      end
    end
  end
end

Asciidoctor::Extensions.register do
  block_macro Nxs::StudioMacro
  docinfo_processor Nxs::StudioAssets.new(location: :head)
  docinfo_processor Nxs::StudioAssets.new(location: :footer)
end
