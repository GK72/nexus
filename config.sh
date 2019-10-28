curl -L https://github.com/gokcehan/lf/releases/download/r13/lf-linux-amd64.tar.gz | tar xzC ~/.local/bin
chmod +x lf
sudo mv lf /usr/local/bin
# https://github.com/gokcehan/lf/wiki/Tutorial


# NeoVIM
sudo apt install neovim
mkdir ~/.config/nvim
touch ~/.config/nvim/init.vim

# Plugin Manager
curl -fLo ~/.local/share/nvim/site/autoload/plug.vim --create-dirs \
https://raw.githubusercontent.com/junegunn/vim-plug/master/plug.vim

# Plugins
echo "
call plug#begin('~/.local/share/nvim/plugged')

Plug 'davidhalter/jedi-vim'                                         " Python auto-completion
Plug 'Shougo/deoplete.nvim', { 'do': ':UpdateRemotePlugins' }
Plug 'vim-airline/vim-airline'
Plug 'vim-airline/vim-airline-themes'
Plug 'jiangmiao/auto-pairs'
Plug 'scrooloose/nerdtree'
Plug 'neomake/neomake'                                              " Code syntax checker
Plug 'terryma/vim-multiple-cursors'
Plug 'machakann/vim-highlightedyank'
Plug 'tmhedberg/SimpylFold'

call plug#end()
" >> ~/.config/init.vim

let g:deoplete#enable_at_startup = 1

./nvim.appimage --appimage-extract
sudo cp -r ~/squashfs-root/usr/* /usr/