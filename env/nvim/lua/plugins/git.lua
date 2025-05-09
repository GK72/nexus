return {
    { 'airblade/vim-gitgutter' },
    {
        'tpope/vim-fugitive',
        config = function()
            vim.keymap.set("n", "<leader>gs", ":Gstatus<CR>")
            vim.keymap.set("n", "<leader>gc", ":Gcommit<CR>")
            vim.keymap.set("n", "<leader>gd", ":Gdiffsplit<CR>")
            vim.keymap.set("n", "<leader>gdd", ":Gvdiffsplit!<CR>")
            vim.keymap.set("n", "<leader>gp", ":GitGutterPreviewHunk<CR>")
            vim.keymap.set("n", "<leader>gf", ":GitGutterFold<CR>")
            vim.keymap.set("n", "<leader>gy", ":GitGutterStageHunk<CR>")
            vim.keymap.set("n", "<leader>gu", ":GitGutterUndoHunk<CR>")
        end
    },

}
