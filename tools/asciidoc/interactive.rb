# frozen_string_literal: true

# Part of Nexus Library
#
# Interactive blocks: panels of inputs over a chart that the page redraws on every change,
# computed in the browser by a model. "Duplicate" copies a panel with its current values, to
# compare variants (A, B, ...) on one y scale.
#
#   interactive::<model>[<input>=<value>,...,unit=HUF]
#
# A model registers its inputs and its script (see `interactive/interactive.js` for the script):
#
#   Adoc::Interactive.model 'savings', script: File.join(__dir__, 'savings.js'), inputs: [
#     Adoc::Interactive::Input.new('balance', 'Starting balance', :money, default: 0, step: 10_000),
#     Adoc::Interactive::Input.new('rate', 'Yearly interest', :percent, default: 5, max: 100, step: 0.1),
#     Adoc::Interactive::Input.new('real', "In today's money", :check)
#   ]
#
# Input kinds: `money` (labeled with the `unit` of the block), `percent`, `number`, `check`. The
# attributes of the macro set the starting values. A block title becomes the figure caption. Pages
# with a block get the styles (`interactive/interactive.css`), the chart and panel scripts, and the
# scripts of their models.

require 'asciidoctor'
require 'asciidoctor/extensions'
require 'cgi'

module Adoc
  module Interactive
    DIR = File.join(__dir__, 'interactive')

    Input = Struct.new(:name, :label, :kind, :default, :min, :max, :step) do
      def initialize(name, label, kind, default: nil, min: nil, max: nil, step: nil)
        default = false if default.nil? && kind == :check
        min = 0 if min.nil? && kind != :check
        super(name, label, kind, default.nil? ? 0 : default, min, max, step)
      end
    end

    Model = Struct.new(:name, :inputs, :script)

    @models = {}

    class << self
      attr_reader :models

      def model(name, inputs:, script:)
        @models[name] = Model.new(name, inputs, script)
      end

      # The models a page uses, kept on the document: not in its attributes, which Asciidoctor
      # resets before the conversion, where the docinfo is written (an ObjectSpace::WeakMap lost
      # its keys on large pages)
      def used(document)
        document.instance_variable_get(:@nx_models) || document.instance_variable_set(:@nx_models, [])
      end
    end

    module_function

    def esc(text)
      CGI.escapeHTML(text.to_s)
    end

    def html(model, attrs)
      unit = attrs.fetch('unit', '')
      inputs = model.inputs.map { |input| field(input, attrs.fetch(input.name, input.default).to_s, unit) }
      <<~HTML
        <div class="ix" data-model="#{esc(model.name)}" data-unit="#{esc(unit)}">
        <section class="ix-panel">
        <div class="ix-head"><span class="ix-name">A</span>
        <button type="button" class="ix-dup">Duplicate</button>
        <button type="button" class="ix-remove" aria-label="Remove" hidden>Remove</button></div>
        <form class="ix-inputs">#{inputs.join("\n")}</form>
        <div class="ix-summary" aria-live="polite"></div>
        <div class="chart ix-chart"><noscript>The chart needs JavaScript.</noscript></div>
        </section>
        </div>
      HTML
    end

    def field(input, value, unit)
      if input.kind == :check
        checked = %w[true yes 1].include?(value) ? ' checked' : ''
        return %(<label class="ix-check"><input type="checkbox" name="#{esc(input.name)}"#{checked}><span>#{esc(input.label)}</span></label>)
      end

      suffix = { money: unit, percent: '%' }[input.kind]
      limits = { 'min' => input.min, 'max' => input.max, 'step' => input.step }.compact.map { |k, v| %(#{k}="#{v}") }.join(' ')
      <<~HTML.strip
        <label class="ix-input"><span>#{esc(input.label)}</span>
        <span class="ix-field"><input type="number" name="#{esc(input.name)}" value="#{esc(value.delete('%,_ '))}" #{limits} data-kind="#{input.kind}" inputmode="decimal">#{suffix && !suffix.empty? ? %(<span class="ix-unit">#{esc(suffix)}</span>) : ''}</span></label>
      HTML
    end

    # The block in a figure card, the title as the numbered caption.
    def figure(parent, body, title)
      html = ['<div class="imageblock figure">', '<div class="content">', body, '</div>']
      if title
        doc = parent.document
        number = doc.increment_and_store_counter('figure-number', parent)
        html << %(<div class="title">#{doc.attributes['figure-caption']} #{number}. #{parent.apply_title_subs(title)}</div>)
      end
      html << '</div>'
      html.join("\n")
    end
  end

  # `interactive::<model>[<input>=<value>,...]`
  class InteractiveMacro < Asciidoctor::Extensions::BlockMacroProcessor
    use_dsl
    named :interactive

    def process(parent, target, attrs)
      model = Interactive.models[target] or
        raise ArgumentError, "interactive: unknown model `#{target}` (registered: #{Interactive.models.keys.join(', ')})"

      doc = parent.document
      used = Interactive.used(doc)
      used << model unless used.include?(model)
      create_pass_block(parent, Interactive.figure(parent, Interactive.html(model, attrs), attrs['title']), {}, subs: nil)
    end
  end

  # The styles in <head>; the chart and panel scripts, then those of the models, at the end of <body>.
  class InteractiveAssets < Asciidoctor::Extensions::DocinfoProcessor
    use_dsl

    def process(document)
      models = Interactive.used(document)
      return if models.empty?

      if config[:location] == :head
        "<style>\n#{File.read(File.join(Interactive::DIR, 'interactive.css'))}</style>"
      else
        scripts = %w[chart.js interactive.js].map { |f| File.join(Interactive::DIR, f) } + models.map(&:script)
        scripts.map { |f| "<script>\n#{File.read(f)}</script>" }.join("\n")
      end
    end
  end
end

Asciidoctor::Extensions.register do
  block_macro Adoc::InteractiveMacro
  docinfo_processor Adoc::InteractiveAssets.new(location: :head)
  docinfo_processor Adoc::InteractiveAssets.new(location: :footer)
end
