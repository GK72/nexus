# frozen_string_literal: true

# Part of Nexus Library
#
# The AsciiDoc extensions of `tools/doc.sh`, registered on require:
#
#   theme.rb          the stylesheet of the pages (`$ADOC_CSS`)
#   interactive.rb    `interactive::<model>[]`, inputs over a chart computed in the browser

require_relative 'theme'
require_relative 'interactive'
