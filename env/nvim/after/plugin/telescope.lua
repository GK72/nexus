local builtin = require('telescope.builtin')

vim.keymap.set("n", "<leader>p", builtin.find_files)
vim.keymap.set("n", "<leader>a", builtin.live_grep)
vim.keymap.set("n", "<leader>s", builtin.grep_string)
vim.keymap.set("n", "<leader>b", builtin.buffers)
vim.keymap.set("n", "<leader>h", builtin.help_tags)

vim.keymap.set("n", "<leader>fo", builtin.oldfiles)
vim.keymap.set("n", "<leader>fc", builtin.command_history)
vim.keymap.set("n", "<leader>fm", builtin.marks)
vim.keymap.set("n", "<leader>fq", builtin.quickfix)

vim.keymap.set("n", "<leader>ft", builtin.treesitter)
vim.keymap.set("n", "<leader>fr", builtin.lsp_references)
vim.keymap.set("n", "<leader>fd", builtin.lsp_document_symbols)
vim.keymap.set("n", "<leader>fs", builtin.lsp_workspace_symbols)

vim.keymap.set("n", "<leader>fgc", builtin.git_commits)
vim.keymap.set("n", "<leader>fgs", builtin.git_status)
vim.keymap.set("n", "<leader>fgb", builtin.git_branches)
