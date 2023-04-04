local lsp = require('lsp-zero')

local lspkind = require'lspkind'
lspkind.init()

lsp.preset('recommended')

-- Fix Undefined global 'vim'
lsp.configure('sumneko_lua', {
    settings = {
        Lua = {
            diagnostics = {
                globals = { 'vim' }
            }
        }
    }
})

local cmp = require('cmp')
local cmp_select = { behavior = cmp.SelectBehavior.Select }

local cmp_mappings = lsp.defaults.cmp_mappings({
    ['<C-d>'] = cmp.mapping.scroll_docs(-4),
    ['<C-f>'] = cmp.mapping.scroll_docs(4),
    ['<C-Space>'] = cmp.mapping.complete(),
    ['<C-e>'] = cmp.mapping.confirm({ select = true }),
    ['<C-p>'] = cmp.mapping.select_prev_item(cmp_select),
    ['<C-n>'] = cmp.mapping.select_next_item(cmp_select),
})

cmp_mappings['<CR>'] = nil
cmp_mappings['<Tab>'] = nil
cmp_mappings['<S-Tab>'] = nil

local cmp_sources = {
   { name = 'nvim_lsp' },
   { name = 'path' },
   { name = 'buffer' , keyword_length = 3 },
   { name = 'tmux',
       option = {
           all_panes = true,
           label = '[tmux]'
       }
   },
}

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

lsp.setup_nvim_cmp({
    mapping = cmp_mappings,
    sources = cmp_sources,
    formatting = cmp_formatting,
})

lsp.on_attach(function(client, bufnr)
    local opts = { buffer = bufnr, remap = false }

    vim.keymap.set("n", "<leader>o", ":ClangdSwitchSourceHeader<CR>")
    vim.keymap.set("n", "<leader>d", vim.lsp.buf.definition, opts)
    vim.keymap.set("n", "<leader>e", vim.lsp.buf.signature_help, opts)
    vim.keymap.set("n", "<leader>ls", vim.lsp.buf.workspace_symbol, opts)
    vim.keymap.set("n", "<leader>w", vim.diagnostic.open_float, opts)
end)

lsp.setup()

vim.diagnostic.config({
    virtual_text = true,
})
