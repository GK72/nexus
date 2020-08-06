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

function! EpochToDateTime()
    normal! yiw
    let epoch = getreg('')
    execute "!date --date=@" . epoch[0:9]
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
