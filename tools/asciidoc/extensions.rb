# frozen_string_literal: true

# Part of Nexus Library
#
# The AsciiDoc extensions of `tools/doc.sh`, registered on require:
#
#   theme.rb          the stylesheet of the pages: tokens and a style level (`:theme-style:`)
#   icons.rb          `i:<name>[]`, the SVG icons of `res/adoc/icons/` and `$ADOC_ICONS`
#   interactive.rb    `interactive::<model>[]`, inputs over a chart computed in the browser
#   studio.rb         `studio::color[]`, `studio::font[]`, editors of the theme tokens

require_relative 'theme'
require_relative 'icons'
require_relative 'interactive'
require_relative 'studio'
