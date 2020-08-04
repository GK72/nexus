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
alias gch='git checkout'
alias gd='git diff'
alias gdc='git diff --cached'
alias gds='git --no-pager diff --stat'
alias gdjq='git-diff-jq'
alias gl="git --no-pager log --decorate --graph --date=format:'%a %d %b %Y %H:%M:%S' --pretty=tformat:'%C(178)%h %C(029)%cd %C(027)%<(20,trunc)%cn %C(250)%s %C(auto)%d' -n10"
alias gp='git push origin HEAD:refs/for/master'
alias gs='git status'

alias clearcache='sudo sh -c "echo 1 > /proc/sys/vm/drop_caches"'

alias build='~/repos/nexus/tools/build'
alias datef=~/repos/nexus/tools/dateformat.sh

# Builders

alias btledger='build -p ~/repos/nexus/.worktrees/feat-ui -r bin/test_nexus && build -p ~/repos/ledger -- -DPATH_NEXUS_REPO=~/repos/nexus/.worktrees/feat-ui && ~/repos/ledger/build/debug-clang11/bin/test_ledger'
alias bledger='build -p ~/repos/nexus/.worktrees/feat-ui -r bin/test_nexus && build -p ~/repos/ledger -- -DPATH_NEXUS_REPO=~/repos/nexus/.worktrees/feat-ui && ~/repos/ledger/build/debug-clang11/bin/ledger'
