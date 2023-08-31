require('lualine').setup {
    options = {
        icons_enabled = true,
        theme = 'auto',
        component_separators = { left = '', right = ''},
        section_separators = { left = '', right = ''},
    },
    tabline = {
        lualine_a = {
            {
                'buffers',
                show_filename_only = true,
                hide_filename_extension = false,
                show_modified_status = true,

                mode = 4,       -- buffer name + buffer number
                max_length = vim.o.columns * 2 / 3,
                use_mode_colors = false,

                symbols = {
                    modified = ' ●',
                    alternate_file = '#',
                    directory = '',
                },
            }
        }
    }
}
