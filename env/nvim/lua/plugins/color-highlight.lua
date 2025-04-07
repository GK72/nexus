return {
    'brenoprata10/nvim-highlight-colors',
    config = function()
        require('nvim-highlight-colors').setup {
            render = "virtual",
            virtual_symbol_position = "eol",
            virtual_symbol = "●",                       -- u+25cf
            virtual_symbol_suffix = " color ███",       -- u+2588
            enable_hex = true,
            enable_short_hex = false,
            enable_rgb = true,
            enable_hsl = true,
            enable_ansi = true,
        }
    end
}
