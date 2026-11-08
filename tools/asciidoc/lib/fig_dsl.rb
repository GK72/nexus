# frozen_string_literal: true

# Part of Nexus Library
#
# Library: `Nxs::FigDsl`, the line notation for box-and-arrow figures, compiled to `Nxs::Fig` calls
# (`plugins/figure.rb`).
#
#   aria: what the figure shows          (required, lines are joined)
#   row 30 h=54                          y and height of the indented boxes below
#     cam @260 w=170 camera2d :green | ECS component | :orange → NDC
#   box @20,150 w=170 h=66 title         a box outside a row
#   a -> b : label                       arrow; `-->` dashed, `<->` and `<-->` two heads
#   a.bottom-40 -> b.top :muted : label @510,118
#   # comment
#
# A box is `name @x[,y] w= [h=] title [:role] | line | line`; lines are muted unless they start with
# a role. Arrows without a side connect facing sides: left/right when the boxes share rows of
# pixels, top/bottom otherwise. Arrows on the same side of a box are spread evenly along it; an
# offset (`.bottom-40`) is measured from the side's center. `@x,y` moves a label.

require_relative 'fig'

module Nxs
  class FigDsl
    Box = Struct.new(:name, :x, :y, :w, :h) do
      def cx = x + (w / 2.0)
      def cy = y + (h / 2.0)
    end
    End = Struct.new(:box, :side, :offset, :point)
    Edge = Struct.new(:from, :to, :heads, :dash, :role, :label, :label_at, :line)

    NUM = '-?\d+(?:\.\d+)?'
    ROW = /\Arow\s+(?<y>#{NUM})(?:\s+h=(?<h>#{NUM}))?\z/
    BOX = /\A(?:box\s+)?(?<name>[a-z_]\w*)\s+@(?<x>#{NUM})(?:,(?<y>#{NUM}))?(?<size>(?:\s+[wh]=#{NUM})+)\s+(?<text>\S.*)\z/
    ANCHOR = /(?<box>[a-z_]\w*)(?:\.(?<side>top|bottom|left|right)(?<off>[+-]\d+(?:\.\d+)?)?)?/
    EDGE = /\A(?<a>#{ANCHOR})\s+(?<op><-->|<->|-->|->)\s+(?<b>#{ANCHOR.source.gsub('?<', '?<b_')})
            (?:\s+:(?<role>[a-z]+))?(?:\s+:\s+(?<label>.*?))?(?:\s+@(?<lx>#{NUM}),(?<ly>#{NUM}))?\z/x
    GAP = 2                                     # space between an arrow head and its box
    LEAN = 20                                   # horizontal run of an arrow that counts as leaning

    def self.render(code, id:, width:, height:, file: '(fig)', line: 1)
      new(id, file, line).compile(code, width: width, height: height)
    end

    def initialize(id, file, line)
      @id = id
      @file = file
      @line = line
      @boxes = {}
      @edges = []
      @aria = []
    end

    def compile(code, width:, height:)
      parse(code)
      @n = @line
      fail!('missing `aria:` line') if @aria.empty?
      route
      Fig.build(id: @id, width: width, height: height, label: @aria.join(' ')) do |f|
        @boxes.each_value { |b| draw_box(f, b) }
        @edges.each { |e| draw_edge(f, e) }
      end
    end

    private

    # ----------------------------------------------------------------------- Parsing

    def parse(code)
      row = nil
      code.each_line.with_index(@line) do |raw, n|
        @n = n
        text = raw.rstrip
        next if text.strip.empty? || text.lstrip.start_with?('#')

        row = nil unless text.start_with?(' ', "\t")
        text = text.strip
        if (m = text.match(/\Aaria:\s*(.*)\z/))
          @aria << m[1]
        elsif (m = ROW.match(text))
          row = { y: m[:y].to_f, h: m[:h]&.to_f }
        elsif (m = EDGE.match(text))
          @edges << edge(m)
        elsif (m = BOX.match(text))
          box(m, row)
        else
          fail!("cannot parse: #{text}")
        end
      end
    end

    def box(m, row)
      size = m[:size].scan(/([wh])=(#{NUM})/o).to_h { |k, v| [k.to_sym, v.to_f] }
      y = m[:y]&.to_f || row&.dig(:y) || fail!("#{m[:name]}: no y, and not in a row")
      h = size[:h] || row&.dig(:h) || fail!("#{m[:name]}: no h, and the row has none")
      fail!("#{m[:name]}: no w") unless size[:w]
      fail!("#{m[:name]}: defined twice") if @boxes.key?(m[:name])

      title, *lines = m[:text].split(/\s*\|\s*/)
      title, title_role = role_suffix(title)
      b = Box.new(m[:name], m[:x].to_f, y, size[:w], h)
      @boxes[b.name] = [b, title, title_role, lines.map { |l| role_prefix(l) }]
    end

    def edge(m)
      Edge.new(anchor(m[:box], m[:side], m[:off]), anchor(m[:b_box], m[:b_side], m[:b_off]),
               m[:op].start_with?('<') ? :both : :end, m[:op].include?('--') ? '5 4' : nil,
               role(m[:role] || 'ink'), m[:label], m[:lx] && [m[:lx].to_f, m[:ly].to_f], @n)
    end

    def anchor(name, side, off)
      End.new(name, side&.to_sym, off&.to_f, nil)
    end

    def role_suffix(text)
      m = text.match(/\A(.*?)\s+:([a-z]+)\z/)
      m && Fig::ROLES.key?(m[2].to_sym) ? [m[1], m[2].to_sym] : [text, :ink]
    end

    def role_prefix(text)
      m = text.match(/\A:([a-z]+)\s+(.*)\z/)
      m && Fig::ROLES.key?(m[1].to_sym) ? [m[1].to_sym, m[2]] : [:muted, text]
    end

    def role(name)
      Fig::ROLES.key?(name.to_sym) ? name.to_sym : fail!("unknown role: #{name}")
    end

    def fail!(msg)
      raise ArgumentError, "#{@file}:#{@n}: fig #{@id}: #{msg}"
    end

    # ----------------------------------------------------------------------- Routing

    def route
      @edges.each do |e|
        @n = e.line
        a = box_of(e.from)
        b = box_of(e.to)
        horizontal = a.y < b.y + b.h && b.y < a.y + a.h
        e.from.side ||= horizontal ? (b.cx > a.cx ? :right : :left) : (b.cy > a.cy ? :bottom : :top)
        e.to.side ||= { right: :left, left: :right, bottom: :top, top: :bottom }[e.from.side]
      end
      ends = @edges.flat_map { |e| [[e.from, e.to], [e.to, e.from]] }
      ends.group_by { |en, _| [en.box, en.side] }.each_value do |list|
        side = list.first[0].side
        spread = list.reject { |en, _| en.offset }.sort_by { |_, other| along(box_of(other), side) }
        spread.each_with_index { |(en, _), i| en.point = point(en, (i + 1).fdiv(spread.size + 1)) }
        list.select { |en, _| en.offset }.each { |en, _| en.point = point(en, nil) }
      end
    end

    def box_of(en)
      @boxes.fetch(en.box) { fail!("unknown box: #{en.box}") }[0]
    end

    # Coordinate of a box center along a side: x for top/bottom, y for left/right.
    def along(box, side) = %i[top bottom].include?(side) ? box.cx : box.cy

    # Point on the side at `t` (0..1) of its length, or at the explicit offset from its center.
    def point(en, t)
      b = box_of(en)
      horizontal_side = %i[top bottom].include?(en.side)
      len = horizontal_side ? b.w : b.h
      pos = t ? t * len : (len / 2.0) + en.offset
      case en.side
      when :top then [b.x + pos, b.y]
      when :bottom then [b.x + pos, b.y + b.h]
      when :left then [b.x, b.y + pos]
      else [b.x + b.w, b.y + pos]
      end
    end

    # ----------------------------------------------------------------------- Drawing

    def draw_box(f, (b, title, title_role, lines))
      f.node(b.x, b.y, b.w, b.h, title, *lines, title_role: title_role)
    end

    def draw_edge(f, e)
      from = e.from.point
      to = e.to.point
      dx = to[0] - from[0]
      dy = to[1] - from[1]
      len = Math.hypot(dx, dy)
      back = ->(p, sign) { [p[0] + (sign * GAP * dx / len), p[1] + (sign * GAP * dy / len)] }
      to = back[to, -1]
      from = back[from, 1] if e.heads == :both
      f.arrow(from, to, role: e.role, dash: e.dash, head: e.heads)
      label(f, e, from, to, dx) if e.label
    end

    # Above an arrow between left and right sides, beside one between top and bottom on the side it leans
    # to; right of a nearly vertical one.
    def label(f, e, from, to, dx)
      mid = [(from[0] + to[0]) / 2.0, (from[1] + to[1]) / 2.0]
      at, anchor = if %i[left right].include?(e.from.side) then [[mid[0], mid[1] - 8], 'middle']
                   elsif dx < -LEAN then [[mid[0] - 12, mid[1] + 4], 'end']
                   else [[mid[0] + 10, mid[1] + 4], 'start']
                   end
      f.text(e.label_at || at, e.label, role: e.role, anchor: anchor, size: 11)
    end
  end
end
