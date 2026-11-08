# frozen_string_literal: true

# Part of Nexus Library
#
# Plugin: `i:<name>[]`, an icon in the text, in the color of the text.
#
#   i:copy[]                 decorative icon, hidden from screen readers
#   i:copy[label="Copy"]     icon that stands for a word
#
# The icons and their CSS are `Nxs::Icons` (`lib/icons.rb`), embedded by the theme.

require 'asciidoctor'
require 'asciidoctor/extensions'
require 'cgi'

require_relative '../lib/icons'

module Nxs
  class IconMacro < Asciidoctor::Extensions::InlineMacroProcessor
    include Asciidoctor::Logging
    use_dsl
    named :i
    name_positional_attributes 'label'

    def process(parent, target, attrs)
      unless Icons.files.key?(target)
        logger.warn "i: unknown icon `#{target}` (icons: #{Icons.files.keys.join(', ')})"
        return create_inline parent, :quoted, target
      end
      label = attrs['label']
      a11y = label ? %(role="img" aria-label="#{CGI.escapeHTML(label)}") : 'aria-hidden="true"'
      create_inline parent, :quoted, %(<span class="icon #{target}" #{a11y}></span>)
    end
  end
end

Asciidoctor::Extensions.register do
  inline_macro Nxs::IconMacro
end
