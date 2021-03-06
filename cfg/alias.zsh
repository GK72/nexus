alias ls='ls --color=auto'
alias ll='ls -lh'
alias la='ls -A'
alias l='ls -CF'

alias grep='grep --color=auto'
alias fgrep='fgrep --color=auto'
alias egrep='egrep --color=auto'

alias tws='timew start'
alias twt='timew stop'
alias twss='timew summary'
alias twc='timew continue'

alias rsyncp="rsync -avz --info=progress2"

alias gap='git add --patch'
alias gc='git commit'
alias gca='git commit --amend'
alias gcp='git cherry-pick'
alias gch='git checkout'
alias gd='git diff'
alias gdc='git diff --cached'
alias gds='git --no-pager diff --stat'
alias gdjq='git-diff-jq'
alias gl="git --no-pager log --decorate --graph --date=format:'%a %d %b %Y %H:%M:%S' --pretty=tformat:'%C(178)%h %C(029)%cd %C(027)%<(20,trunc)%cn %C(250)%s %C(auto)%d' -n10"
alias gp='git push origin HEAD:refs/for/master'
alias gs='git status'
alias gri='git rebase -i HEAD~10'

alias clearcache='sudo sh -c "echo 1 > /proc/sys/vm/drop_caches"'

alias build='~/repos/nexus/tools/build.sh'
alias datef=~/repos/nexus/tools/dateformat.sh

alias wls='~/repos/nexus/build/release-gcc/bin/worklog show summary'
alias wli='~/repos/nexus/build/release-gcc/bin/worklog start'
alias wlo='~/repos/nexus/build/release-gcc/bin/worklog end'
