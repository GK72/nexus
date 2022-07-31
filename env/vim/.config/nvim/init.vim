" ---------------------------------------==[ Plugins ]==--------------------------------------------
call plug#begin('~/.local/share/nvim/plugged')
    " ---= LSP and completion
    Plug 'neovim/nvim-lspconfig'
    Plug 'nvim-lua/completion-nvim'
    Plug 'hrsh7th/nvim-cmp'
    Plug 'hrsh7th/cmp-nvim-lsp'
    Plug 'hrsh7th/cmp-buffer'
    Plug 'hrsh7th/cmp-path'
    Plug 'hrsh7th/cmp-vsnip'
    Plug 'hrsh7th/vim-vsnip'
    Plug 'andersevenrud/cmp-tmux'

    Plug 'onsails/lspkind-nvim'                 " Pictograms for completion

    Plug 'nvim-treesitter/nvim-treesitter', {'do': ':TSUpdate'}
    Plug 'nvim-treesitter/playground'
    Plug 'nvim-lua/popup.nvim'
    Plug 'nvim-lua/plenary.nvim'
    Plug 'nvim-telescope/telescope.nvim'
    Plug 'nvim-telescope/telescope-symbols.nvim'

    " ---= Text editing
    Plug 'tpope/vim-surround'                   " Surround text with quotes, brackets
    Plug 'preservim/nerdcommenter'
    Plug 'gu-fan/riv.vim'
    Plug 'gu-fan/InstantRst'
    Plug 'chrisbra/csv.vim'
    Plug 'godlygeek/tabular'

    Plug 'lervag/vimtex'

    " ---= Visuals
    Plug 'vim-airline/vim-airline'
    Plug 'vim-airline/vim-airline-themes'
    Plug 'sainnhe/edge'

    Plug 'ryanoasis/vim-devicons'               " File type icons
    Plug 'machakann/vim-highlightedyank'        " Brief highlight of yanked range
    Plug 'tmhedberg/SimpylFold'                 " Folding for Python
    Plug 'rickhowe/diffchar.vim'                " Detailed diff

    Plug 'scrooloose/nerdtree'                  " File explorer
    Plug 'preservim/tagbar'                     " Tag browser

    Plug 'iamcco/markdown-preview.nvim', { 'do': 'cd app && yarn install'  }

    " ---= Completion and searching
    Plug 'junegunn/fzf', { 'do': { -> fzf#install() } }

    " ---= Git
    Plug 'airblade/vim-gitgutter'
    Plug 'tpope/vim-fugitive'

    " ---= Misc
    Plug 'vim-scripts/L9'
    Plug 'vim-scripts/ReplaceWithRegister'

    Plug 'mhinz/vim-startify'
    Plug 'junegunn/goyo.vim'
call plug#end()


" -----------------------------==[ Custom Plugins and Functions ]==---------------------------------

source ~/.config/nvim/arrownavigation.vim
source ~/.config/nvim/dragvisuals.vim
source ~/.config/nvim/vmath.vim
source ~/.config/nvim/functions.vim

" ------------------------------------==[ Basic Settings ]==----------------------------------------

let g:term_id = 0
let g:edge_style = 'aura'
let g:vimtex_view_general_viewer = 'okular'
let g:vimtex_view_general_options = '--unique file:@pdf\#src:@line@tex'
let g:vimtex_view_general_options_latexmk = '--unique'
let g:vimtex_fold_enabled = "true"

set t_Co=256
color edge

" set termguicolors

if &encoding != 'utf-8'
    set encoding=utf-8
endif"

" ---= Visuals
set background=dark
set cc=120                  " column border marker
set cursorline              " highlight current line
set hlsearch                " highlight search results
set lcs=trail:·,tab:»·      " visualize tabs and trailing spaces (list chars)
set list
set number                  " add line numbers
set relativenumber          " relative line numbers

set scrolloff=8             " Start scrolling when we're 8 lines away from margins
set showcmd                 " show command in bottom bar
set showmatch               " show matching brackets

" ---= Formatting
set autoindent              " indent a new line the same amount as the line just typed
set smartindent
set expandtab               " converts tabs to white space
set shiftwidth=4            " width for autoindents
set softtabstop=4           " see multiple spaces as tabstops so <BS> does the right thing
set tabstop=4               " number of columns occupied by a tab character

" ---= Command line completion
set wildmenu
set wildmode=longest:full,full

" ---= Misc
set path+=**
set ignorecase              " case insensitive matching

"set copyindent              " copy indent from the previous line

filetype plugin indent on   " allows auto-indenting depending on file type
syntax on                   " syntax highlighting

set mouse=

" ------------------------------------==[ Configuration ]==-----------------------------------------

let g:deoplete#enable_at_startup = 1
let g:airline_powerline_fonts=1
let g:airline#extensions#tabline#enabled = 1
let g:airline#extensions#tabline#buffer_nr_show = 1

" ---= Doxygen
let g:doxygen_enhanced_color=1
let g:load_doxygen_syntax=1

set completeopt=menuone,noinsert,noselect
let g:completion_matching_strategy_list = ['exact', 'substring', 'fuzzy']

let g:goyo_width = 120
let g:NERDSpaceDelims = 1

set undofile
let g:fzf_layout = { 'window' : { 'width': 0.8, 'height': 0.8 } }
let $FZF_DEFAULT_OPTS='--reverse'

" ---------------------------------------==[ Mappings ]==-------------------------------------------

let mapleader = ' '

" ---= Exit insert mode with 'jj'
inoremap jj <ESC>

" ---= Command without shift
nnoremap ; :
nnoremap : ,
nnoremap , ;

" ---= Terminal: exit key
tnoremap <leader>q <C-\><C-n>
nnoremap <silent> <leader>q :call CloseTerminal()<CR>
nnoremap <silent> <leader>t :call RunInTerminal("n")<CR>
vnoremap <silent> <leader>t :call RunInTerminal("v")<CR>

" ---= Windows
nnoremap <leader>v <C-w>v<C-w>l
noremap <leader>r :res +10<CR>
noremap <leader>R :res -10<CR>
noremap <leader>vr :vert res +5<CR>
noremap <leader>vR :vert res -5<CR>

" ---= Git
noremap <leader>gs :Gstatus<CR>
noremap <leader>gc :Gcommit<CR>
noremap <leader>gd :Gdiffsplit<CR>
noremap <leader>gdd :Gvdiffsplit!<CR>
noremap <leader>gp :GitGutterPreviewHunk<CR>
noremap <leader>gf :GitGutterFold<CR>
noremap <leader>gy :GitGutterStageHunk<CR>
noremap <leader>gu :GitGutterUndoHunk<CR>

" ---= Searching
nnoremap <leader>p <cmd>lua require('telescope.builtin').find_files()<cr>
nnoremap <leader>a <cmd>lua require('telescope.builtin').live_grep()<cr>
nnoremap <leader>s <cmd>lua require('telescope.builtin').grep_string()<cr>
nnoremap <leader>b <cmd>lua require('telescope.builtin').buffers{ show_all_buffers = true }<cr>
nnoremap <leader>h <cmd>lua require('telescope.builtin').help_tags()<cr>

nnoremap <leader>fo <cmd>lua require('telescope.builtin').oldfiles()<cr>
nnoremap <leader>fc <cmd>lua require('telescope.builtin').command_history()<cr>
nnoremap <leader>fm <cmd>lua require('telescope.builtin').marks()<cr>
nnoremap <leader>fq <cmd>lua require('telescope.builtin').quickfix()<cr>

nnoremap <leader>ft <cmd>lua require('telescope.builtin').treesitter()<cr>
nnoremap <leader>fr <cmd>lua require('telescope.builtin').lsp_references()<cr>
nnoremap <leader>fd <cmd>lua require('telescope.builtin').lsp_document_symbols()<cr>
nnoremap <leader>fs <cmd>lua require('telescope.builtin').lsp_workspace_symbols()<cr>

nnoremap <leader>fgc <cmd>lua require('telescope.builtin').git_commits()<cr>
nnoremap <leader>fgs <cmd>lua require('telescope.builtin').git_status()<cr>
nnoremap <leader>fgb <cmd>lua require('telescope.builtin').git_branches()<cr>

" ---= Navigation
noremap <leader>o :ClangdSwitchSourceHeader<CR>
noremap <leader>d :lua vim.lsp.buf.definition()<CR>
noremap <leader>e :lua vim.lsp.buf.signature_help()<CR>
noremap <leader>ls :lua vim.lsp.buf.workspace_symbol()<CR>

nnoremap <leader>w :lua vim.lsp.diagnostic.show_line_diagnostics()<CR>

" ---= Moving text
vnoremap J :m '>+1<CR>gv=gv
vnoremap K :m '<-2<CR>gv=gv

" ---= Misc
noremap <leader>n :NERDTreeToggle<CR>
noremap <leader>m :TagbarToggle<CR>
noremap <leader>no :nohlsearch<CR>

vmap  <expr>  <LEFT>   DVB_Drag('left')
vmap  <expr>  <RIGHT>  DVB_Drag('right')
vmap  <expr>  <DOWN>   DVB_Drag('down')
vmap  <expr>  <UP>     DVB_Drag('up')
vmap  <expr>  D        DVB_Duplicate()

xmap <silent><expr>  ++  VMATH_YankAndAnalyse()
nmap <silent>        ++  vip++

" ---= Command mappings
command! Cmm call CommentAlignMiddle()
command! Ce call Godbolt()

" ------------------------------------==[ Auto Commands ]==-----------------------------------------

autocmd FileType git set foldmethod=syntax
autocmd FileType go setlocal noexpandtab

" ---= Goyo config
function! s:goyo_enter()
    set noshowmode
    set noshowcmd
endfunction

function! s:goyo_leave()
    highlight cursorline cterm=bold ctermbg=234
    highlight colorcolumn ctermbg=235
endfunction

function! TrimWhitespace()
    let l:save = winsaveview()
    keeppatterns %s/\s\+$//e
    keeppatterns %s/\($\n\s*\)\+\%$//e
    call winrestview(l:save)
endfunction

augroup GGROUP
    autocmd!
    autocmd! User GoyoEnter nested call <SID>goyo_enter()
    autocmd! User GoyoLeave nested call <SID>goyo_leave()

    autocmd BufWritePre * :call TrimWhitespace()
augroup END

" --------------------------------------==[ Highlights ]==------------------------------------------

highlight! link Comment Grey
highlight! link DiffText White

highlight! link TSAttribute Black
highlight! link TSFunction Green
highlight! link TSNamespace Cyan
highlight! link TSNumber Red
highlight! link TSParameter Grey
highlight! link TSParameterReference Grey
highlight! link TSString Green
highlight! link TSText Red
highlight! link TSMath Red
highlight! link TSVariable White

highlight cursorline cterm=bold ctermbg=236
highlight colorcolumn ctermbg=233
highlight VertSplit ctermfg=237 ctermbg=234

highlight normal ctermfg=white

" ------------------------------==[ Treesitter and LSP config ]==-----------------------------------

lua require'lspconfig'.clangd.setup{ on_attach=require'completion'.on_attach }
lua require'lspconfig'.jedi_language_server.setup{ on_attach=require'completion'.on_attach }
lua require'lspconfig'.rls.setup{ on_attach=require'completion'.on_attach }

lua require'lspconfig'.bashls.setup{ filetypes = { "sh", "zsh" }, on_attach=require'completion'.on_attach }
lua require'lspconfig'.dockerls.setup{ on_attach=require'completion'.on_attach }
lua require'lspconfig'.gopls.setup{ on_attach=require'completion'.on_attach }
lua require'lspconfig'.vimls.setup{ on_attach=require'completion'.on_attach }
lua require'lspconfig'.hls.setup{ on_attach=require'completion'.on_attach }
lua require'lspconfig'.yamlls.setup{}
lua require'lspconfig'.texlab.setup{}

lua << EOF

require("nxv")

-- LSP config

local nvim_lsp = require'lspconfig'

local on_attach = function(client)
    require'completion'.on_attach(client)
end

nvim_lsp.gopls.setup {
    cmd = {"gopls", "serve"},
    settings = {
        gopls = {
            analyses = {
                unusedparams = true,
            },
                staticcheck = true,
        },
    },
}

nvim_lsp.rls.setup({
    -- cmd = { "rustup", "run", "nightly", "rls" },
    settings = {
        rust = {
            unstable_features = true,
            build_on_save = false,
            all_features = true,
        },
    },
})

require'lspconfig'.jsonls.setup {
    commands = {
        Format = {
            function()
                vim.lsp.buf.range_formatting({},{0,0},{vim.fn.line("$"),0})
            end
        }
    },
    on_attach = require'completion'.on_attach
}

require'nvim-treesitter.configs'.setup {
    -- Modules and its options go here
    highlight = { enable = true },
    incremental_selection = { enable = true },
    textobjects = { enable = true },
}

-- Completion

local lspkind = require'lspkind'
lspkind.init()
local cmp = require'cmp'

cmp.setup({
    snippet = {
        expand = function(args)
        vim.fn["vsnip#anonymous"](args.body)
        end,
    },

    mapping = {
        ['<C-d>'] = cmp.mapping.scroll_docs(-4),
        ['<C-f>'] = cmp.mapping.scroll_docs(4),
        ['<C-Space>'] = cmp.mapping.complete(),
        --['<C-e>'] = cmp.mapping.close(),
        ['<C-e>'] = cmp.mapping.confirm({ select = true }),
        ['<C-n>'] = function(fallback)
          if cmp.visible() then
            cmp.select_next_item()
          else
            fallback()
          end
         end,
         ['<C-p>'] = function(fallback)
           if cmp.visible() then
             cmp.select_prev_item()
           else
             fallback()
           end
         end
    },
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
    formatting = {
        format = lspkind.cmp_format {
            with_text = true,
            menu = {
                nvim_lsp = "[LSP]",
                path = "[path]",
                buffer = "[buf]",
                tmux = "[tmux]",
            }
        }
    },
    experimental = {
        native_menu = false,
        ghost_text = true
    }
})

-- Setup lspconfig.
require('lspconfig')["clangd"].setup {
    capabilities = require('cmp_nvim_lsp').update_capabilities(vim.lsp.protocol.make_client_capabilities())
}
require('lspconfig')["jedi_language_server"].setup {
    capabilities = require('cmp_nvim_lsp').update_capabilities(vim.lsp.protocol.make_client_capabilities())
}
require('lspconfig')["bashls"].setup {
    capabilities = require('cmp_nvim_lsp').update_capabilities(vim.lsp.protocol.make_client_capabilities())
}
require('lspconfig')["vimls"].setup {
    capabilities = require('cmp_nvim_lsp').update_capabilities(vim.lsp.protocol.make_client_capabilities())
}
require('lspconfig')["dockerls"].setup {
    capabilities = require('cmp_nvim_lsp').update_capabilities(vim.lsp.protocol.make_client_capabilities())
}
require('lspconfig')["gopls"].setup {
    capabilities = require('cmp_nvim_lsp').update_capabilities(vim.lsp.protocol.make_client_capabilities())
}
require('lspconfig')["hls"].setup {
    capabilities = require('cmp_nvim_lsp').update_capabilities(vim.lsp.protocol.make_client_capabilities())
}
require('lspconfig')["yamlls"].setup {
    capabilities = require('cmp_nvim_lsp').update_capabilities(vim.lsp.protocol.make_client_capabilities())
}
require('lspconfig')["texlab"].setup {
    capabilities = require('cmp_nvim_lsp').update_capabilities(vim.lsp.protocol.make_client_capabilities())
}

local system_name
if vim.fn.has("mac") == 1 then
    system_name = "macOS"
elseif vim.fn.has("unix") == 1 then
    system_name = "Linux"
elseif vim.fn.has('win32') == 1 then
    system_name = "Windows"
else
    print("Unsupported system for sumneko")
end

-- set the path to the sumneko installation; if you previously installed via the now deprecated :LspInstall, use
local sumneko_root_path = "/usr/lib/lua-language-server"
local sumneko_binary = sumneko_root_path.."/bin/".."/lua-language-server"

-- require'lspconfig'.sumneko_lua.setup {
--     -- cmd = {sumneko_binary, "-E", sumneko_root_path .. "/main.lua"};
--     settings = {
--         Lua = {
--             runtime = {
--                 -- Tell the language server which version of Lua you're using (most likely LuaJIT in the case of Neovim)
--                 version = 'LuaJIT',
--             },
--             diagnostics = {
--                 -- Get the language server to recognize the `vim` global
--                 globals = {'vim'},
--             },
--             workspace = {
--                 -- Make the server aware of Neovim runtime files
--                 library = vim.api.nvim_get_runtime_file("", true),
--             },
--             telemetry = {
--                 enable = false,
--             },
--         },
--     },
--     on_attach = require'completion'.on_attach
-- }
EOF

" ---------------------------------------==[ Startify ]==-------------------------------------------

let g:startify_commands = [
    \ {'t': ['Terminal', ': term']}
    \ ]

let g:tagbar_type_rst = {
            \ 'ctagstype' : 'ReStructuredText',
            \ 'kinds' : [
            \     'c:chapter',
            \     's:article',
            \     'S:section',
            \ ],
            \ "sort" : 0
            \ }
