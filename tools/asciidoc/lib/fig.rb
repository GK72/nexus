# frozen_string_literal: true

# Part of Nexus Library
#
# Library: `Nxs::Fig`, the builder of the inline SVG figures (`plugins/figure.rb`).
#
# Colors are roles, not values: a role maps to the theme classes of `res/adoc/styles/plain.css`
# (`st-*` stroke, `t-*` text, `fill-*` fill), so the figures follow the light and dark theme. Points
# are `[x, y]` in viewBox units. Every primitive takes `attrs:` for extra SVG attributes, and `raw`
# adds any markup, so a figure can do whatever hand-written SVG can.

require 'cgi'

module Nxs
  class Fig
    # role => [stroke class, text class, fill class]
    ROLES = {
      ink: %w[st-ink] + [nil, 'fill-ink'],
      muted: %w[st-muted t-muted fill-muted],
      grid: ['st-grid', nil, nil],
      red: %w[st-red t-red fill-red],
      orange: %w[st-orange t-orange fill-orange],
      green: %w[st-green t-green fill-green],
      teal: %w[st-teal t-teal fill-teal],
      blue: %w[st-blue t-blue fill-blue],
      purple: %w[st-purple t-purple fill-purple]
    }.freeze

    # Builds a figure; the block receives the figure, the result is the SVG markup.
    def self.build(id:, width:, height:, label: nil, &block)
      fig = new(id, width, height, label)
      block.call(fig)
      fig.to_svg
    end

    # Runs a Ruby figure body in its own scope, with the figure as `f`; returns the SVG markup.
    def self.run(code, id:, width:, height:, file: '(fig)', line: 1)
      build(id: id, width: width, height: height) { |f| eval(code, body_binding(f), file, line) } # rubocop:disable Security/Eval
    end

    def self.body_binding(f) # rubocop:disable Naming/MethodParameterName
      binding
    end
    private_class_method :body_binding

    attr_reader :width, :height
    attr_accessor :label

    def initialize(id, width, height, label)
      @id = id
      @width = width
      @height = height
      @label = label
      @defs = {}
      @body = []
    end

    # ----------------------------------------------------------------------- Primitives

    # `fill:` is a style, as the stroke classes set `fill: none`, which would win over an attribute.
    def rect(x, y, w, h, role: :ink, stroke: 1.5, dash: nil, rx: nil, fill: nil, attrs: {})
      add(:rect, { x: x, y: y, width: w, height: h, rx: rx, style: fill && "fill: #{fill}" }, stroke_attrs(role, stroke, dash), attrs)
    end

    def line(from, to, role: :ink, stroke: 1.5, dash: nil, attrs: {})
      add(:line, { x1: from[0], y1: from[1], x2: to[0], y2: to[1] }, stroke_attrs(role, stroke, dash), attrs)
    end

    # `head:` is `:end`, `:start` or `:both`.
    def arrow(from, to, role: :ink, stroke: 1.5, dash: nil, head: :end, attrs: {})
      line(from, to, role: role, stroke: stroke, dash: dash, attrs: heads(role, head).merge(attrs))
    end

    # Path with the SVG `d` syntax, e.g., a curved arrow with `head:`.
    def path(d, role: :ink, stroke: 1.5, dash: nil, head: nil, attrs: {})
      add(:path, { d: d }, stroke_attrs(role, stroke, dash), head ? heads(role, head) : {}, attrs)
    end

    def circle(at, r, role: :ink, attrs: {})
      add(:circle, { cx: at[0], cy: at[1], r: r, class: cls(role, 2) }, attrs)
    end

    # `anchor:` is `start`, `middle` or `end`; `rotate:` turns the text around its position.
    def text(at, str, role: :ink, size: nil, anchor: nil, weight: nil, rotate: nil, title: false, attrs: {})
      classes = [title ? 't-title' : nil, cls(role, 1)].compact.join(' ')
      pos = rotate ? { transform: "translate(#{num(at[0])} #{num(at[1])}) rotate(#{num(rotate)})" } : { x: at[0], y: at[1] }
      a = pos.merge('text-anchor': anchor, class: classes.empty? ? nil : classes, 'font-size': size, 'font-weight': weight)
      @body << tag(:text, a.merge(attrs), CGI.escapeHTML(str))
      self
    end

    # Grid pattern aligned to `origin`; returns the fill value for `rect(fill:)`.
    def grid(cell, origin: [0, 0])
      pid = "#{@id}-grid-#{num(cell).tr('.', '_')}"
      @defs[pid] ||= tag(:pattern, { id: pid, width: cell, height: cell, patternUnits: 'userSpaceOnUse', x: origin[0], y: origin[1] },
                         tag(:path, { d: "M#{num(cell)} 0 L0 0 0 #{num(cell)}", class: 'st-grid', 'stroke-width': 1 }))
      "url(##{pid})"
    end

    def raw(markup)
      @body << markup
      self
    end

    def comment(str)
      @body << "<!-- #{str} -->"
      self
    end

    # ----------------------------------------------------------------------- Composites

    # Box with a title line and muted (or `[role, text]`) lines below, centered.
    def node(x, y, w, h, title, *lines, title_role: :ink)
      rect(x, y, w, h, rx: 6, role: nil, stroke: nil, attrs: { class: 'box' })
      cx = x + (w / 2.0)
      text([cx, y + 23], title, role: title_role, anchor: 'middle', weight: 500)
      lines.each_with_index do |l, i|
        role, str = l.is_a?(Array) ? l : [:muted, l]
        text([cx, y + 41 + (16 * i)], str, role: role, anchor: 'middle', size: 11)
      end
      self
    end

    # Dimension bracket between two points with end ticks, and an optional label beside it.
    # `side:` is the label's side: `:left`/`:right` for a vertical bracket, `:above`/`:below` otherwise.
    def bracket(from, to, label: nil, role: :ink, tick: 5, side: nil, gap: 12, size: 11)
      line(from, to, role: role, stroke: 1)
      vertical = from[0] == to[0]
      [from, to].each do |p|
        vertical ? line([p[0] - tick, p[1]], [p[0] + tick, p[1]], role: role, stroke: 1)
                 : line([p[0], p[1] - tick], [p[0], p[1] + tick], role: role, stroke: 1)
      end
      return self unless label

      mid = [(from[0] + to[0]) / 2.0, (from[1] + to[1]) / 2.0]
      if vertical
        dx = side == :right ? gap : -gap
        text([mid[0] + dx, mid[1]], label, role: role, anchor: 'middle', size: size, rotate: -90)
      else
        dy = side == :below ? gap + 4 : -gap + 4
        text([mid[0], mid[1] + dy], label, role: role, anchor: 'middle', size: size)
      end
    end

    # ----------------------------------------------------------------------- Output

    def to_svg
      raise ArgumentError, "fig: #{@id}: missing aria label" if @label.to_s.strip.empty?

      head = tag(:svg, { xmlns: 'http://www.w3.org/2000/svg', class: 'fig', viewBox: "0 0 #{num(@width)} #{num(@height)}",
                         role: 'img', 'aria-label': @label }, nil, open: true)
      defs = @defs.empty? ? [] : ['  <defs>', *@defs.values.map { |d| "    #{d}" }, '  </defs>']
      [head, *defs, *@body.map { |b| "  #{b}" }, '</svg>'].join("\n")
    end

    private

    def add(name, *attr_sets)
      @body << tag(name, attr_sets.reduce({}, :merge))
      self
    end

    def cls(role, index)
      return nil if role.nil?

      ROLES.fetch(role) { raise ArgumentError, "fig: unknown role: #{role}" }[index]
    end

    def stroke_attrs(role, stroke, dash)
      { class: cls(role, 0), 'stroke-width': stroke, 'stroke-dasharray': dash }
    end

    def heads(role, head)
      url = "url(##{marker(role)})"
      { 'marker-start': %i[start both].include?(head) ? url : nil, 'marker-end': %i[end both].include?(head) ? url : nil }
    end

    # Arrow head in the color of the role, defined once per figure.
    def marker(role)
      mid = "#{@id}-ah-#{role}"
      @defs[mid] ||= tag(:marker, { id: mid, viewBox: '0 0 10 10', refX: 9, refY: 5, markerWidth: 7, markerHeight: 7,
                                    orient: 'auto-start-reverse' },
                         tag(:path, { d: 'M0 0 L10 5 L0 10 z', class: cls(role, 2) || 'fill-ink' }))
      mid
    end

    def tag(name, attrs, content = nil, open: false)
      a = attrs.reject { |_, v| v.nil? }.map { |k, v| %(#{k}="#{v.is_a?(Numeric) ? num(v) : CGI.escapeHTML(v.to_s)}") }
      start = "<#{name}#{a.empty? ? '' : " #{a.join(' ')}"}"
      return "#{start}>" if open
      return "#{start}/>" if content.nil?

      "#{start}>#{content}</#{name}>"
    end

    # Numbers rounded to 0.01, without a trailing `.0`.
    def num(v)
      r = v.round(2)
      r == r.to_i ? r.to_i.to_s : r.to_s
    end
  end
end
