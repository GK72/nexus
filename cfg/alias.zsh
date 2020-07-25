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

alias gl='git --no-pager log --oneline --decorate --graph -n10'

alias clearcache='sudo sh -c "echo 1 > /proc/sys/vm/drop_caches"'

alias build='~/repos/nexus/tools/build'
alias datef=~/repos/nexus/tools/dateformat.sh

# Builders

alias bledger='build -p ~/repos/nexus/.worktrees/feat-ui -r bin/test_nexus && ~/repos/nexus/tools/build -p ~/repos/ledger -- -DPATH_NEXUS_REPO=~/repos/nexus/.worktrees/feat-ui && ~/repos/ledger/build/debug-clang11/bin/ledger'
