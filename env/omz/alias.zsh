alias ls='ls --color=auto'
alias ll='ls -lh'
alias la='ls -A'
alias l='ls -CF'

alias grep='grep --color=auto'
alias fgrep='fgrep --color=auto'
alias egrep='egrep --color=auto'

alias ag='ag --color-match "0;36"'

alias rsyncp="rsync -avz --info=progress2"

alias gap='git add --patch'
alias gbv='git --no-pager branch -v'
alias gc='git commit'
alias gca='git commit --amend --no-edit'
alias gcam='git commit --amend'
alias gch='git checkout'
alias gcp='git cherry-pick'
alias gd='git diff'
alias gdc='git diff --cached'
alias gds='git --no-pager diff --stat'
alias gdjq='git-diff-jq'
alias gl="git --no-pager log --decorate --graph --date=format:'%a %d %b %Y %H:%M:%S' --pretty=tformat:'%C(178)%h %C(029)%cd %C(027)%<(20,trunc)%cn %C(250)%s %C(auto)%d' -n10"
alias gp='git push origin HEAD:refs/for/master'
alias gs='git status'
alias gri='git rebase -i HEAD~10'
alias gwl='git worktree list'

alias clearcache='sudo sh -c "echo 1 > /proc/sys/vm/drop_caches"'

alias build='~/.local/bin/build.sh'

alias tws='timew start'
alias twt='timew stop'
alias twss='timew summary'
alias twc='timew continue'

alias wls='~/.local/bin/worklog show summary'
alias wli='~/.local/bin/worklog start'
alias wlo='~/.local/bin/worklog end'
