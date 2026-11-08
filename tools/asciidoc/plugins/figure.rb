# frozen_string_literal: true

# Part of Nexus Library
#
# Plugin: `[fig]` blocks, inline SVG figures built with `Nxs::Fig` (`lib/fig.rb`). A block title
# becomes the numbered figure caption, like on `image::[role=figure]`.
#
#   [fig,name,880x232]         box-and-arrow notation (`lib/fig_dsl.rb`) in a `....` block
#   [fig,name,880x300,ruby]    Ruby body with the figure as `f`; set `f.label` for the aria label
#
# `name` prefixes the ids inside the SVG, so it must be unique per page. The feature set, with
# examples: `doc/documenting/pages/figures.adoc`.

require 'asciidoctor'
require 'asciidoctor/extensions'

require_relative '../lib/fig'
require_relative '../lib/fig_dsl'

module Nxs
  module Figure
    module_function

    def html(parent, svg, title)
      html = ['<div class="imageblock figure">', '<div class="content">', svg, '</div>']
      if title
        doc = parent.document
        number = doc.increment_and_store_counter('figure-number', parent)
        html << %(<div class="title">#{doc.attributes['figure-caption']} #{number}. #{parent.apply_title_subs(title)}</div>)
      end
      html << '</div>'
      html.join("\n")
    end
  end

  class FigureBlock < Asciidoctor::Extensions::BlockProcessor
    use_dsl
    named :fig
    on_contexts :literal, :listing
    name_positional_attributes 'name', 'size', 'lang'

    def process(parent, reader, attrs)
      name = attrs['name'] or raise ArgumentError, 'fig: missing name, e.g. [fig,camera2d-frame,880x232]'
      size = attrs['size'].to_s.match(/\A(\d+)x(\d+)\z/) or raise ArgumentError, "fig: #{name}: size is not WxH"
      cursor = reader.cursor
      where = { id: name, width: size[1].to_i, height: size[2].to_i, file: cursor.path, line: cursor.lineno }
      svg = case attrs['lang']
            when nil then FigDsl.render(reader.read, **where)
            when 'ruby' then Fig.run(reader.read, **where)
            else raise ArgumentError, "fig: #{name}: unknown language: #{attrs['lang']}"
            end
      create_pass_block(parent, Figure.html(parent, svg, attrs['title']), {}, subs: nil)
    end
  end
end

Asciidoctor::Extensions.register do
  block Nxs::FigureBlock
end
