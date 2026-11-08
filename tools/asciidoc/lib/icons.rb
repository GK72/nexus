# frozen_string_literal: true

# Part of Nexus Library
#
# Library: `Nxs::Icons`, the icons of the pages: one SVG per icon, drawn on a 16×16 grid in black
# strokes, embedded by the theme as CSS masks, `--icon-<name>`, so an icon takes the color of its
# context and the page stays one file.
#
# The icons are `res/adoc/icons/*.svg`, then those of `$ADOC_ICONS` (directories separated by `:`),
# where an icon of the same name replaces the earlier one. A new icon needs only its file: the
# `span.icon.<name>` rules of `i:<name>[]` (`plugins/icons.rb`) are generated with the properties.

module Nxs
  module Icons
    DIR = File.expand_path('../../../res/adoc/icons', __dir__)

    def self.dirs
      [DIR] + ENV.fetch('ADOC_ICONS', '').split(':').reject(&:empty?)
    end

    # Name => file; later directories win
    def self.files
      @files ||= dirs.each_with_object({}) do |dir, out|
        Dir.glob(File.join(dir, '*.svg')).sort.each do |f|
          name = File.basename(f, '.svg')
          name.match?(/\A[a-z0-9-]+\z/) or raise ArgumentError, "icons: bad icon name `#{name}` (#{f})"
          out[name] = f
        end
      end
    end

    # Percent-encodes everything but the characters a data URI keeps as they are
    def self.encode(svg)
      svg.gsub(%r{[^A-Za-z0-9\-_.~'=/:,]}) { |c| c.bytes.map { |b| format('%%%02X', b) }.join }
    end

    # The properties with the icons as data URIs, and the rules of `i:<name>[]`
    def self.css
      props = files.map { |name, f| "  --icon-#{name}: url(\"data:image/svg+xml,#{encode(File.read(f).strip)}\");" }
      rules = files.keys.map { |name| "span.icon.#{name} { --icon: var(--icon-#{name}); }" }
      <<~CSS
        :root {
        #{props.join("\n")}
        }
        span.icon {
          display: inline-block; width: 1em; height: 1em; vertical-align: -0.14em;
          background: currentColor; mask: var(--icon) center / contain no-repeat;
        }
        #{rules.join("\n")}
      CSS
    end
  end
end
