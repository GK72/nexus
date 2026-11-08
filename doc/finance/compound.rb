# frozen_string_literal: true

# Inputs of the `compound` model of `compound.adoc` (computed in `compound.js`); loaded by
# `tools/doc.sh` as the companion of the document.

Nxs::Interactive.model 'compound', script: File.join(__dir__, 'compound.js'), inputs: [
  Nxs::Interactive::Input.new('balance', 'Starting balance', :money, step: 10_000),
  Nxs::Interactive::Input.new('rate', 'Yearly interest', :percent, default: 5, max: 100, step: 0.1),
  Nxs::Interactive::Input.new('inflation', 'Inflation a year', :percent, default: 3, min: -20, max: 100, step: 0.1),
  Nxs::Interactive::Input.new('salary', 'Salary a month', :money, step: 10_000),
  Nxs::Interactive::Input.new('raise', 'Raise a year', :percent, min: -50, max: 100, step: 0.5),
  Nxs::Interactive::Input.new('expenses', 'Expenses a month', :money, step: 10_000),
  Nxs::Interactive::Input.new('deposit', 'Deposit a month', :money, step: 10_000),
  Nxs::Interactive::Input.new('loan', 'Loan', :money, step: 10_000),
  Nxs::Interactive::Input.new('loan-rate', 'Loan interest', :percent, default: 10, max: 100, step: 0.1),
  Nxs::Interactive::Input.new('term', 'Loan years', :number, default: 5, max: 40, step: 1),
  Nxs::Interactive::Input.new('years', 'Years shown', :number, default: 3, min: 1, max: 30, step: 1),
  Nxs::Interactive::Input.new('real', "In today's money", :check)
]
