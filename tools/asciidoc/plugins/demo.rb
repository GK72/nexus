# frozen_string_literal: true

# Part of Nexus Library
#
# Plugin: `[demo]` blocks, AsciiDoc shown twice, as its source and rendered, from one copy.
#
#   [demo]
#   --
#   .A titled figure.
#   [fig,name,880x200]
#   ....
#   ...
#   ....
#   --
#
# The block is an open block (`--`) or, when the example holds an open block itself, an example
# block (`====`). Titles and attributes go inside, onto the example; the demo adds no frame.

require 'asciidoctor'
require 'asciidoctor/extensions'

module Nxs
  class DemoBlock < Asciidoctor::Extensions::BlockProcessor
    use_dsl
    named :demo
    on_contexts :open, :example
    parse_content_as :raw

    def process(parent, reader, attrs)
      lines = reader.read_lines
      demo = create_open_block(parent, nil, attrs.merge('role' => ['demo', attrs['role']].compact.join(' ')))
      source = create_listing_block(demo, lines, { 'style' => 'source', 'language' => 'asciidoc' })
      source.style = 'source' # the parser sets it on parsed blocks; the converter reads it
      demo << source
      parse_content(demo, lines)
      demo
    end
  end
end

Asciidoctor::Extensions.register do
  block Nxs::DemoBlock
end
