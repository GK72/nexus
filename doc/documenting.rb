# frozen_string_literal: true

# Companion of `documenting.adoc`: the `line` model of its interactive example.

Adoc::Interactive.model 'line', script: File.join(__dir__, 'documenting.js'), inputs: [
  Adoc::Interactive::Input.new('a', 'a', :number, default: 1, min: -10, max: 10, step: 0.5),
  Adoc::Interactive::Input.new('b', 'b', :number, min: -10, max: 10, step: 0.5)
]
