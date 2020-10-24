" Stylize the text into a comment with center alignment
function! CommentAlignMiddle()
    center 100
    normal! hhv0r-e3hce==[
    normal! A ]==
    normal! 50A-
    normal! d100|
endfunction

func Eatchar(pat)
  let c = nr2char(getchar(0))
  return (c =~ a:pat) ? '' : c
endfunc

" Test: 1588012345
function! EpochToDateTime()
    normal! yiw
    let epoch = getreg('')
    let datetime = system("date --date=@" . epoch[0:9])
    echo datetime

    " Dev: autoreload
    " lua for k in pairs(package.loaded) do if k:match("^epoch%-to%-date") then package.loaded[k] = nil end end
    " lua require("epoch-to-date").display()
endfunction

function! CloseTerminal()
    if g:term_id == 0
        echo "No opened terminal!"
    else
        call chanclose(g:term_id)
        let g:term_id = 0
    endif
endfunction

function! OpenTerminal()
    execute "split"
    execute "terminal"
    let g:term_id = [b:terminal_job_id, bufnr('%')]
    wincmd J
    wincmd k
    wincmd w
endfunction

function! SendTerminal(cmd)
    silent! let ret = chansend(g:term_id[0], [a:cmd, ""])
    if ret == 0
        call OpenTerminal()
        call chansend(g:term_id[0], [a:cmd, ""])
    endif
endfunction

function! RepoPath()
    let cf = expand('%:p')
    let repos = stridx(cf, "repos")
    let repoNameStart = stridx(cf, "/", repos + 2)
    let repoNameEnd = stridx(cf, "/", repoNameStart + 1)
    return cf[0:repoNameEnd-1]
endfunction

function! Build(target)
    call SendTerminal("build -p " . RepoPath() . " -t " . a:target)
endfunction

function! BuildAndRun(target)
    call SendTerminal("build -p " . RepoPath() . " -t " . a:target . " -r")
endfunction

function! RunInTerminal(m)
    call OpenTerminal()

    if a:m == "n"
        normal! Y
    elseif a:m == "v"
        normal! Y
    endif

    let buffer = getreg('')
    let lines = split(buffer, "\n")
    call filter(lines, 'v:val != ""')

    if a:m == "n" && len(lines) > 0
        let lines[0] = trim(lines[0])
    endif

    call jobsend(g:term_id, lines)
    call jobsend(g:term_id, "\n")
endfunction

function! InsertBashArgs()
    execute "read $HOME/.local/share/nvim/templates/bashArgs.sh"
endfunction

function! ReadEpochToDateTime()
    normal! yiw
    let epoch = getreg('')
    execute "read !date --date=@" . epoch[0:9]
endfunction

function! ClangCheckImpl(cmd)
    if &autowrite | wall | endif
    echo "Running " . a:cmd . " ..."
    let l:output = system(a:cmd)
    cexpr l:output
    cwindow
    let w:quickfix_title = a:cmd
    if v:shell_error != 0
        cc
    endif
    let g:clang_check_last_cmd = a:cmd
endfunction

function! ClangCheck()
    let l:filename = expand('%')
    if l:filename =~ '\.\(cpp\|cxx\|cc\|c\)$'
        call ClangCheckImpl("clang-check " . l:filename)
    elseif exists("g:clang_check_last_cmd")
        call ClangCheckImpl(g:clang_check_last_cmd)
    else
        echo "Can't detect file's compilation arguments and no previous clang-check invocation!"
    endif
endfunction
