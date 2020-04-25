" ---------------------------------------==[ Plugins ]==--------------------------------------------
call plug#begin('~/.local/share/nvim/plugged')
    " ---= Basics
    Plug 'vim-airline/vim-airline'
    Plug 'vim-airline/vim-airline-themes'
    Plug 'christoomey/vim-sort-motion'          " Sorting using text objects and motions (map: gs)
    Plug 'neomake/neomake'                      " For async running
    Plug 'scrooloose/nerdtree'                  " File explorer

    " ---= Text editing
    Plug 'jiangmiao/auto-pairs'                 " Insert or delete brackets, parens, quotes in pair
    Plug 'tpope/vim-surround'                   " Surround text with quotes, brackets

    " ---= Visuals
    Plug 'machakann/vim-highlightedyank'        " Brief highlight of yanked range
    Plug 'octol/vim-cpp-enhanced-highlight'     " Additional C++ syntax coloring
    Plug 'ryanoasis/vim-devicons'               " File type icons
    Plug 'tmhedberg/SimpylFold'                 " Folding for Python

    " ---= Completion and searching
    Plug 'davidhalter/jedi-vim'                 " Python auto-completion
    Plug 'rdnetto/YCM-Generator', { 'branch': 'stable' }
    Plug 'Valloric/YouCompleteMe', { 'do': './install.py --clang-completer'  }
    Plug 'junegunn/fzf', { 'do': { -> fzf#install() } }
    Plug 'junegunn/fzf.vim'

    Plug 'derekwyatt/vim-fswitch'               " Switching between source and header files
    Plug 'szw/vim-tags'                         " Ctag generator (TagsGenerate)

    " ---= Git
    Plug 'airblade/vim-gitgutter'
    Plug 'tpope/vim-fugitive'

    " ---= Misc
    Plug 'vim-syntastic/syntastic'              " Syntax checker
    Plug 'vim-scripts/L9'
    Plug 'vim-scripts/ReplaceWithRegister'
call plug#end()


" -----------------------------==[ Custom Plugins and Functions ]==---------------------------------
source ~/.local/share/nvim/arrownavigation.vim
source ~/.local/share/nvim/dragvisuals.vim
source ~/.local/share/nvim/vmath.vim
source ~/.local/share/nvim/functions.vim


" ------------------------------------==[ Basic Settings ]==----------------------------------------

color desert

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

"set termguicolors
"set copyindent              " copy indent from the previous line

filetype plugin indent on   " allows auto-indenting depending on file type
syntax on                   " syntax highlighting

" Some coloring
highlight cursorline cterm=bold ctermbg=234
highlight colorcolumn ctermbg=235


" ------------------------------------==[ Configuration ]==-----------------------------------------
let g:deoplete#enable_at_startup = 1
let g:airline_powerline_fonts=1
let g:airline#extensions#tabline#enabled = 1
let g:airline#extensions#tabline#buffer_nr_show = 1

" ---= C++ Syntax Highlighter
let g:cpp_class_scope_highlight = 1         " Class scope
let g:cpp_member_variable_highlight = 1     " Member variables
let g:cpp_class_decl_highlight = 1          " Class names
let g:cpp_posix_standard = 1                " POSIX functions
let g:cpp_experimental_template_highlight = 1
" let g:cpp_experimental_simple_template_highlight = 1

" ---= Doxygen
let g:doxygen_enhanced_color=1
let g:load_doxygen_syntax=1

" ---= NerdTree
" noremap <C-n> :NERDTreeToggle<CR>     " ctrl+n open/closes nerd tree
" let g:NERDTreeQuitOnOpen = 1          " quit nerd tree on file open
" let g:NERDTreeWinPos = "right""
" autocmd vimenter * NERDTree
" autocmd VimEnter * wincmd p
" autocmd bufenter * if (winnr("$") == 1 && exists("b:NERDTree") && b:NERDTree.isTabTree()) | q | endif

" ---= YouCompleteMe
let g:ycm_show_diagnostics_ui = 0
let g:ycm_key_list_select_completion = ['<C-k>', '<Down>']
let g:ycm_key_list_previous_completion = ['<C-l>', '<Up>']
let g:SuperTabDefaulCompletionType = '<C-k>'

" disable annoying ycm confirmation
let g:ycm_confirm_extra_conf = 0

" add path to ycm_extra_conf.py (you could also copy the file in the home folder)
" delete '...98' argument from .ycm_extra_conf.py, otherwise syntastic does not work properly
let g:ycm_global_ycm_extra_conf = '/home/gkpro/.vim/plugged/YouCompleteMe/third_party/ycmd/cpp/ycm/.ycm_extra_conf.py'


" ---------------------------------------==[ Mappings ]==-------------------------------------------
" ---= Command without shift
nnoremap ; :
nnoremap : ,
nnoremap , ;

" ---= Terminal: Use ESC to exit insert mode
tnoremap <C-\><C-\> <C-\><C-n>

vmap  <expr>  <LEFT>   DVB_Drag('left')
vmap  <expr>  <RIGHT>  DVB_Drag('right')
vmap  <expr>  <DOWN>   DVB_Drag('down')
vmap  <expr>  <UP>     DVB_Drag('up')
vmap  <expr>  D        DVB_Duplicate()

xmap <silent><expr>  ++  VMATH_YankAndAnalyse()
nmap <silent>        ++  vip++

nmap <silent> <F5> :call ClangCheck()<CR><CR>
