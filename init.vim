call plug#begin('~/.local/share/nvim/plugged')

Plug 'davidhalter/jedi-vim'                                         " Python auto-completion
Plug 'Valloric/YouCompleteMe', { 'do': './install.py --clang-completer'  }
Plug 'vim-airline/vim-airline'
Plug 'vim-airline/vim-airline-themes'
Plug 'jiangmiao/auto-pairs'
Plug 'scrooloose/nerdtree'                                          " File explorer
Plug 'neomake/neomake'                                              " For async running
Plug 'terryma/vim-multiple-cursors'
Plug 'machakann/vim-highlightedyank'
Plug 'tmhedberg/SimpylFold'
Plug 'octol/vim-cpp-enhanced-highlight'
Plug 'vim-scripts/a.vim'
Plug 'rhysd/git-messenger.vim'
Plug 'tpope/vim-surround'
Plug 'tpope/vim-repeat'
Plug 'tpope/vim-commentary'
Plug 'christoomey/vim-sort-motion'
Plug 'vim-scripts/FuzzyFinder'
Plug 'vim-scripts/L9'
Plug 'vim-scripts/ReplaceWithRegister'
Plug 'airblade/vim-gitgutter'
Plug 'tpope/vim-fugitive'
Plug 'rdnetto/YCM-Generator', { 'branch': 'stable' }

call plug#end()

color desert

set showmatch               " show matching brackets
set ignorecase              " case insensitive matching
set hlsearch                " highlight search results
set tabstop=4               " number of columns occupied by a tab character
set softtabstop=4           " see multiple spaces as tabstops so <BS> does the right thing
set expandtab               " converts tabs to white space
set shiftwidth=4            " width for autoindents
set autoindent              " indent a new line the same amount as the line just typed
set copyindent              " copy indent from the previous line
set number                  " add line numbers
set wildmode=longest,list   " get bash-like tab completions
set cc=120                  " set  column border for good coding style
set cursorline
set showcmd                 " show command in bottom bar
set background=dark
set scrolloff=8             " Start scrolling when we're 8 lines away from margins
"set termguicolors
set lcs=trail:·,tab:»·
set list
set relativenumber

filetype plugin indent on   " allows auto-indenting depending on file type
syntax on                   " syntax highlighting

let g:deoplete#enable_at_startup = 1
let g:airline_powerline_fonts=1
let g:airline#extensions#tabline#enabled = 1
let g:airline#extensions#tabline#buffer_nr_show = 1
let g:doxygen_enhanced_color=1
let g:load_doxygen_syntax=1
let g:cpp_class_scope_highlight = 1


" ==================== Suggestions ========================
" show wild menu (menu of suggestions) when typing commands in command mode
set path+=**
set wildmenu
set showcmd"

if &encoding != 'utf-8'
    set encoding=utf-8
endif"

" ====================== NERDTree =========================
" noremap <C-n> :NERDTreeToggle<CR>     " ctrl+n open/closes nerd tree
" let g:NERDTreeQuitOnOpen = 1          " quit nerd tree on file open
let g:NERDTreeWinPos = "right""
autocmd vimenter * NERDTree
autocmd VimEnter * wincmd p
autocmd bufenter * if (winnr("$") == 1 && exists("b:NERDTree") && b:NERDTree.isTabTree()) | q | endif

" A - switching between files
nnoremap <F4> :A<CR>                    " header / source
inoremap <F4> <ESC>:A<CR>a
nnoremap <F2> :IH<CR>                   "file under cursor
inoremap <F2> <ESC>:IH<CR>
" Use ESC to exit insert mode in :term
tnoremap <C-\><C-\> <C-\><C-n>

" Some coloring
highlight cursorline cterm=bold ctermbg=234
highlight colorcolumn ctermbg=235

" ################ YouCompleteMe ####################
let g:ycm_show_diagnostics_ui = 0
let g:ycm_key_list_select_completion = ['<C-k>', '<Down>']
let g:ycm_key_list_previous_completion = ['<C-l>', '<Up>']
let g:SuperTabDefaulCompletionType = '<C-k>'

" disable annoying ycm confirmation
let g:ycm_confirm_extra_conf = 0

" add path to ycm_extra_conf.py (you could also copy the file in the home folder)
" delete '...98' argument from .ycm_extra_conf.py, otherwise syntastic does not work properly
let g:ycm_global_ycm_extra_conf = '/home/gkpro/.vim/plugged/YouCompleteMe/third_party/ycmd/cpp/ycm/.ycm_extra_conf.py'
