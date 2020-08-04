" ---------------------------------------==[ Plugins ]==--------------------------------------------
call plug#begin('~/.local/share/nvim/plugged')
    " ---= Basics
    Plug 'vim-airline/vim-airline'
    Plug 'vim-airline/vim-airline-themes'
    Plug 'christoomey/vim-sort-motion'          " Sorting using text objects and motions (map: gs)
    Plug 'neomake/neomake'                      " For async running
    Plug 'scrooloose/nerdtree'                  " File explorer

    " ---= Text editing
    "Plug 'jiangmiao/auto-pairs'                 " Insert or delete brackets, parens, quotes in pair
    Plug 'tpope/vim-surround'                   " Surround text with quotes, brackets
    Plug 'preservim/nerdcommenter'

    " ---= Visuals
    Plug 'machakann/vim-highlightedyank'        " Brief highlight of yanked range
    Plug 'octol/vim-cpp-enhanced-highlight'     " Additional C++ syntax coloring
    Plug 'ryanoasis/vim-devicons'               " File type icons
    Plug 'tmhedberg/SimpylFold'                 " Folding for Python

    Plug 'morhetz/gruvbox'

    " ---= Completion and searching
    Plug 'davidhalter/jedi-vim'                 " Python auto-completion
    Plug 'rust-lang/rust.vim'
    Plug 'rdnetto/YCM-Generator', { 'branch': 'stable' }
    Plug 'Valloric/YouCompleteMe', { 'do': './install.py --clang-completer'  }
    Plug 'junegunn/fzf', { 'do': { -> fzf#install() } }
    Plug 'junegunn/fzf.vim'
    Plug 'dyng/ctrlsf.vim'

    Plug 'derekwyatt/vim-fswitch'               " Switching between source and header files
    Plug 'szw/vim-tags'                         " Ctag generator (TagsGenerate)

    " ---= Git
    Plug 'airblade/vim-gitgutter'
    Plug 'tpope/vim-fugitive'

    " ---= Misc
    Plug 'vim-syntastic/syntastic'              " Syntax checker
    Plug 'vim-scripts/L9'
    Plug 'vim-scripts/ReplaceWithRegister'

    Plug 'junegunn/goyo.vim'
call plug#end()


" -----------------------------==[ Custom Plugins and Functions ]==---------------------------------
source ~/.local/share/nvim/plugin/arrownavigation.vim
source ~/.local/share/nvim/plugin/dragvisuals.vim
source ~/.local/share/nvim/plugin/vmath.vim
source ~/.local/share/nvim/plugin/functions.vim


" ------------------------------------==[ Basic Settings ]==----------------------------------------


let g:gruvbox_contrast_dark='hard'

color gruvbox

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
highlight cursorline cterm=bold ctermbg=235
highlight colorcolumn ctermbg=233
highlight VertSplit ctermfg=237 ctermbg=234

highlight normal ctermfg=white
hi! link String GruvboxAqua

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

set completeopt-=preview

" disable annoying ycm confirmation
let g:ycm_confirm_extra_conf = 0

" add path to ycm_extra_conf.py (you could also copy the file in the home folder)
" delete '...98' argument from .ycm_extra_conf.py, otherwise syntastic does not work properly
let g:ycm_global_ycm_extra_conf = '/home/gkpro/.vim/plugged/YouCompleteMe/third_party/ycmd/cpp/ycm/.ycm_extra_conf.py'

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
noremap <leader>p :CtrlSF<space>
noremap <leader>a :Ag<CR>
noremap <leader>p :FZF<CR>

nnoremap <leader>s :Ag <C-R>=expand("<cword>")<CR><CR>

" ---= Moving text
vnoremap J :m '>+1<CR>gv=gv
vnoremap K :m '<-2<CR>gv=gv

" ---= Misc
noremap <leader>n :NERDTreeToggle<CR>
noremap <leader>no :noh<CR>
noremap <leader>o :FSHere<CR>

vmap  <expr>  <LEFT>   DVB_Drag('left')
vmap  <expr>  <RIGHT>  DVB_Drag('right')
vmap  <expr>  <DOWN>   DVB_Drag('down')
vmap  <expr>  <UP>     DVB_Drag('up')
vmap  <expr>  D        DVB_Duplicate()

xmap <silent><expr>  ++  VMATH_YankAndAnalyse()
nmap <silent>        ++  vip++

nmap <silent> <F5> :call ClangCheck()<CR><CR>

" ---= Command mappings
command! Cmm call CommentAlignMiddle()


" ---= Goyo config
function! s:goyo_enter()
    set noshowmode
    set noshowcmd
endfunction

function! s:goyo_leave()
    highlight cursorline cterm=bold ctermbg=234
    highlight colorcolumn ctermbg=235
endfunction

autocmd! User GoyoEnter nested call <SID>goyo_enter()
autocmd! User GoyoLeave nested call <SID>goyo_leave()
