return {
    'VonHeikemen/lsp-zero.nvim',
    {
        {
            'williamboman/mason.nvim',
            lazy = false,
            opts = {},
        },

        { 'onsails/lspkind-nvim' },              -- Pictograms for completion

        -- Autocompletion
        {
            'hrsh7th/nvim-cmp',
            event = 'InsertEnter',
            config = function()
                local cmp = require('cmp')
                local cmp_select = { behavior = cmp.SelectBehavior.Select }
                local lspkind = require'lspkind'
                lspkind.init()

                local cmp_formatting = {
                   format = lspkind.cmp_format {
                       with_text = true,
                       menu = {
                           nvim_lsp = "[LSP]",
                           path = "[path]",
                           buffer = "[buf]",
                           tmux = "[tmux]",
                       }
                   }
                }

                cmp.setup({
                    sources = {
                        { name = 'nvim_lsp' },
                        { name = 'path' },
                        { name = 'buffer' , keyword_length = 3 },
                        { name = 'tmux',
                            option = {
                               all_panes = true,
                               label = '[tmux]'
                            }
                        },
                    },
                    mapping = cmp.mapping.preset.insert({
                        ['<C-Space>'] = cmp.mapping.complete(),
                        ['<C-d>'] = cmp.mapping.scroll_docs(-4),
                        ['<C-f>'] = cmp.mapping.scroll_docs(4),
                        ['<C-e>'] = cmp.mapping.confirm({ select = true }),
                        ['<C-p>'] = cmp.mapping.select_prev_item(cmp_select),
                        ['<C-n>'] = cmp.mapping.select_next_item(cmp_select),
                    }),
                    snippet = {
                        expand = function(args)
                            vim.snippet.expand(args.body)
                        end,
                    },
                    formatting = cmp_formatting,
                })
            end
        },

        -- LSP
        {
            'neovim/nvim-lspconfig',
            cmd = {'LspInfo', 'LspInstall', 'LspStart'},
            event = {'BufReadPre', 'BufNewFile'},
            dependencies = {
                {'hrsh7th/cmp-nvim-lsp'},
                {'williamboman/mason.nvim'},
                {'williamboman/mason-lspconfig.nvim'},
            },

            init = function()
                vim.opt.signcolumn = 'yes'
            end,

            config = function()
                local lsp_defaults = require('lspconfig').util.default_config

                -- Add cmp_nvim_lsp capabilities settings to lspconfig
                -- This should be executed before you configure any language server
                lsp_defaults.capabilities = vim.tbl_deep_extend(
                  'force',
                  lsp_defaults.capabilities,
                  require('cmp_nvim_lsp').default_capabilities()
                )

                -- LspAttach is where you enable features that only work
                -- if there is a language server active in the file
                vim.api.nvim_create_autocmd('LspAttach', {
                  desc = 'LSP actions',
                  callback = function(event)
                    local opts = {buffer = event.buf}

                    vim.keymap.set('n', '<leader>e', '<cmd>lua vim.lsp.buf.hover()<cr>', opts)
                    vim.keymap.set('n', '<leader>d', '<cmd>lua vim.lsp.buf.definition()<cr>', opts)
                    vim.keymap.set('n', '<leader>D', '<cmd>lua vim.lsp.buf.declaration()<cr>', opts)
                    vim.keymap.set('n', '<leader>r', '<cmd>lua vim.lsp.buf.references()<cr>', opts)
                    vim.keymap.set("n", "<leader>o", ":ClangdSwitchSourceHeader<CR>")
                    vim.keymap.set("n", "<leader>w", vim.diagnostic.open_float, opts)
                    -- vim.keymap.set('n', '<F2>', '<cmd>lua vim.lsp.buf.rename()<cr>', opts)
                    -- vim.keymap.set({'n', 'x'}, '<F3>', '<cmd>lua vim.lsp.buf.format({async = true})<cr>', opts)
                    -- vim.keymap.set('n', 'gi', '<cmd>lua vim.lsp.buf.implementation()<cr>', opts)
                    -- vim.keymap.set('n', 'go', '<cmd>lua vim.lsp.buf.type_definition()<cr>', opts)
                    -- vim.keymap.set('n', 'gs', '<cmd>lua vim.lsp.buf.signature_help()<cr>', opts)
                    -- vim.keymap.set('n', '<F4>', '<cmd>lua vim.lsp.buf.code_action()<cr>', opts)

                    vim.keymap.set("n", "<leader>ls", vim.lsp.buf.workspace_symbol, opts)
                  end,
                })

                require('mason-lspconfig').setup({
                    ensure_installed = {},
                    handlers = {
                        -- this first function is the "default handler"
                        -- it applies to every language server without a "custom handler"
                        function(server_name)
                            require('lspconfig')[server_name].setup({})
                        end,
                    }
                })
            end,

            opts = function()
                ---@class PluginLspOpts
            end
        }, -- LSP

        {
            -- Autocompletion
            { 'hrsh7th/cmp-buffer' },
            { 'hrsh7th/cmp-path' },
            { 'saadparwaiz1/cmp_luasnip' },
            { 'hrsh7th/cmp-nvim-lsp' },
            { 'hrsh7th/cmp-nvim-lua' },
            { 'andersevenrud/cmp-tmux' },

            -- Snippets
            {'L3MON4D3/LuaSnip' },
            { 'rafamadriz/friendly-snippets' },
        }
    }
}
