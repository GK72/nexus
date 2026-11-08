# frozen_string_literal: true

# Part of Nexus Library
#
# Library: `Nxs::Source`, helpers for the plugins of Nexus and of a project: paths of the project
# (`$ADOC_ROOT`, set by `tools/doc.sh`), comment stripping and AsciiDoc table output.

module Nxs
  module Source
    PROJECT_DIR = File.expand_path(ENV.fetch('ADOC_ROOT', Dir.pwd))

    module_function

    # Absolute path of a project-relative target; raises with the target in the message if missing.
    def path(target)
      full = File.expand_path(target, PROJECT_DIR)
      raise ArgumentError, "not found: #{target}" unless File.exist?(full)

      full
    end

    def relative(full)
      full.delete_prefix("#{PROJECT_DIR}/")
    end

    # Blanks out `//` and `/* */` comments, keeping string literals, line breaks and offsets.
    def strip_comments(text)
      out = +''
      i = 0
      state = :code
      while i < text.length
        c = text[i]
        n = text[i + 1]
        case state
        when :code
          if c == '/' && n == '/'
            state = :line
          elsif c == '/' && n == '*'
            state = :block
          elsif c == '"'
            state = :string
          end
        when :line
          state = :code if c == "\n"
        when :block
          if c == '*' && n == '/'
            out << '  '
            i += 2
            state = :code
            next
          end
        when :string
          if c == '\\'
            out << c << (n || '')
            i += 2
            next
          end
          state = :code if c == '"'
        end
        out << (%i[line block].include?(state) && c != "\n" ? ' ' : c)
        i += 1
      end
      out
    end

    # Index of the brace closing the one at `open`, in comment-stripped text.
    def matching_brace(text, open)
      depth = 0
      (open...text.length).each do |i|
        depth += 1 if text[i] == '{'
        depth -= 1 if text[i] == '}'
        return i if depth.zero?
      end
      text.length - 1
    end

    def line_of(text, offset)
      text[0...offset].count("\n") + 1
    end

    # Inline literal monospace; the text is not interpreted as AsciiDoc.
    def code(text)
      text.include?('+') ? "`pass:c[#{text.gsub(']', '\]')}]`" : "`+#{text}+`"
    end

    # Escapes the cell separator, so arbitrary text can go into a table cell.
    def cell(text)
      text.to_s.gsub('|', '\|')
    end

    # AsciiDoc source of a table with a header row.
    def table(cols:, header:, rows:, title: nil, id: nil)
      lines = []
      lines << ".#{title}" if title
      lines << %([#{"##{id}," if id}cols="#{cols}",options=header])
      lines << '|==='
      lines << header.map { |h| "| #{h}" }.join(' ')
      lines << ''
      rows.each do |row|
        row.each { |c| lines << "| #{c}" }
        lines << ''
      end
      lines << '|==='
      lines << ''
      lines
    end
  end
end
