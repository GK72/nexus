vim.opt.mouse = ""

vim.opt.number = true
vim.opt.relativenumber = true

vim.opt.tabstop = 4
vim.opt.softtabstop = 4
vim.opt.shiftwidth = 4
vim.opt.expandtab = true

vim.opt.autoindent = true
vim.opt.smartindent = true

vim.opt.termguicolors = true
vim.opt.colorcolumn = "100"
vim.opt.cursorline = true
vim.opt.listchars = "trail:·,tab:»·"
vim.opt.list = true

vim.opt.wrap = false

vim.opt.undodir = os.getenv("HOME") .. "/.vim/undodir"
vim.opt.undofile = true

vim.opt.incsearch = true
vim.opt.ignorecase = true

vim.opt.scrolloff = 8
vim.opt.signcolumn = "yes"

vim.g.edge_style = 'aura'
vim.g.airline_powerline_fonts=1
vim.g.NERDSpaceDelims = 1
