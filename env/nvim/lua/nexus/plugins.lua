vim.cmd [[packadd packer.nvim]]

return require('packer').startup(function(use)
    use 'wbthomason/packer.nvim'

    use { 'nvim-lualine/lualine.nvim' }
    use { 'sainnhe/edge', config = function() vim.cmd('colorscheme edge') end }

    use { 'nvim-telescope/telescope.nvim', tag = '0.1.0',
        requires = { { 'nvim-lua/plenary.nvim' } }
    }

    use { 'nvim-treesitter/nvim-treesitter', { run = ':TSUpdate' } }
    use { 'nvim-treesitter/playground' }
    use { 'junegunn/fzf', { run = 'fzf#install()' } }
    use { 'mbbill/undotree' }

    use { 'VonHeikemen/lsp-zero.nvim',
        requires = {
            -- LSP Support
            { 'neovim/nvim-lspconfig' },
            { 'williamboman/mason.nvim' },
            { 'williamboman/mason-lspconfig.nvim' },

            -- Autocompletion
            { 'hrsh7th/nvim-cmp' },
            { 'hrsh7th/cmp-buffer' },
            { 'hrsh7th/cmp-path' },
            { 'saadparwaiz1/cmp_luasnip' },
            { 'hrsh7th/cmp-nvim-lsp' },
            { 'hrsh7th/cmp-nvim-lua' },
            { 'andersevenrud/cmp-tmux' },

            -- Snippets
            {'L3MON4D3/LuaSnip' },
            { 'rafamadriz/friendly-snippets' },
            { 'onsails/lspkind-nvim' }              -- Pictograms for completion
        }
    }

    --use { 'airblade/vim-gitgutter' }
    use { 'tpope/vim-fugitive' }

    use { 'preservim/nerdcommenter' }
    use { 'ryanoasis/vim-devicons' }
    use { 'machakann/vim-highlightedyank' }

    use { 'scrooloose/nerdtree' }
    use { 'preservim/tagbar' }

    use { 'mhinz/vim-startify' }
end)
