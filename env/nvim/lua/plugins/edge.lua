return {
    'sainnhe/edge',
    config = function()
        vim.cmd('colorscheme edge')

        vim.g.edge_style = "default"
        vim.g.edge_transparent_background = 0
        vim.g.edge_enable_italic = 1
        vim.g.edge_dim_foreground = 0

        vim.cmd('colorscheme edge')   -- dsdsd

        vim.cmd[[highlight! link Comment Grey]]
        vim.cmd[[highlight! link DiffText White]]

        vim.cmd[[highlight! link TSAttribute Black]]
        vim.cmd[[highlight! link TSFunction Green]]
        vim.cmd[[highlight! link TSNamespace Cyan]]
        vim.cmd[[highlight! link TSNumber Red]]
        vim.cmd[[highlight! link TSParameter Grey]]
        vim.cmd[[highlight! link TSParameterReference Grey]]
        vim.cmd[[highlight! link TSString Green]]
        vim.cmd[[highlight! link TSText Red]]
        vim.cmd[[highlight! link TSMath Red]]
        vim.cmd[[highlight! link TSVariable White]]

        vim.cmd[[highlight cursorline cterm=bold ctermbg=236]]
        vim.cmd[[highlight colorcolumn ctermbg=233]]
        vim.cmd[[highlight VertSplit ctermfg=237 ctermbg=234]]

        vim.cmd[[highlight normal ctermfg=white]]
    end
}
