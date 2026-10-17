return {
    { 'airblade/vim-gitgutter' },
    {
        'tpope/vim-fugitive',
        config = function()
            vim.keymap.set("n", "<leader>gs", ":Git<CR>")
            vim.keymap.set("n", "<leader>gc", ":Git commit<CR>")
            vim.keymap.set("n", "<leader>gd", ":Gdiffsplit<CR>")
            vim.keymap.set("n", "<leader>gdd", ":Gvdiffsplit!<CR>")
            vim.keymap.set("n", "<leader>gp", ":GitGutterPreviewHunk<CR>")
            vim.keymap.set("n", "<leader>gf", ":GitGutterFold<CR>")
            vim.keymap.set("n", "<leader>gy", ":GitGutterStageHunk<CR>")
            vim.keymap.set("n", "<leader>gu", ":GitGutterUndoHunk<CR>")
        end
    },
    {
        'sindrets/diffview.nvim',
        dependencies = { 'nvim-tree/nvim-web-devicons' },
        config = function()
            vim.keymap.set("n", "<leader>gv", ":DiffviewOpen master...HEAD<CR>")
            vim.keymap.set("n", "<leader>gl", ":DiffviewFileHistory --range=master..HEAD<CR>")
            vim.keymap.set("n", "<leader>gh", ":DiffviewFileHistory %<CR>")
            vim.keymap.set("n", "<leader>gq", ":DiffviewClose<CR>")
        end
    },

}
