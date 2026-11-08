# frozen_string_literal: true

# Companion of `interactive.adoc`: the `line` model of its example.

Nxs::Interactive.model 'line', script: File.join(__dir__, 'line.js'), inputs: [
  Nxs::Interactive::Input.new('a', 'a', :number, default: 1, min: -10, max: 10, step: 0.5),
  Nxs::Interactive::Input.new('b', 'b', :number, min: -10, max: 10, step: 0.5)
]
