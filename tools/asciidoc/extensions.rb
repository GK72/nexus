# frozen_string_literal: true

# Part of Nexus Library
#
# The AsciiDoc extensions of `tools/doc.sh`: requiring this file registers every plugin.
#
# plugins/   One file per plugin; requiring it registers its Asciidoctor extensions. Its browser
#            assets sit in a folder of the same name.
#
#   theme.rb          the stylesheet of the pages: tokens, palette, a style level, the icons
#   icons.rb          `i:<name>[]`, an icon in the text
#   figure.rb         `[fig,name,WxH]`, inline SVG figures, in line notation or Ruby
#   interactive.rb    `interactive::<model>[]`, inputs over a chart computed in the browser
#   studio.rb         `studio::color[]`, `studio::font[]`, editors of the theme tokens
#   demo.rb           `[demo]`, AsciiDoc shown as source and rendered
#   nav.rb            breadcrumbs and previous/next on the pages of a book
#   live.rb           live reload under `tools/doc.sh --serve`
#
# lib/       The library of the plugins, `Nxs::*`: plain Ruby that registers nothing. The plugins
#            of a project (`tools/doc.sh -r`) may use it too.
#
#   source.rb         `Nxs::Source`, project paths (`$ADOC_ROOT`) and AsciiDoc table output
#   icons.rb          `Nxs::Icons`, the SVG icons as CSS masks
#   fig.rb            `Nxs::Fig`, the SVG builder of the figures
#   fig_dsl.rb        `Nxs::FigDsl`, the box-and-arrow notation, compiled to `Nxs::Fig`

require_relative 'lib/source'

Dir.glob(File.join(__dir__, 'plugins', '*.rb')).sort.each { |plugin| require plugin }
