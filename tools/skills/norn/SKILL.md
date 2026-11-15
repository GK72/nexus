---
name: norn
description: Work with norn, the author's task service for every project — find a task, start it, add tasks (as the author's or Claude's own), log time, re-estimate, link the Claude session, close or drop it. Use when the user names a task ("let's do the X task", "the Baldr log task"), asks what to work on next or what is open, ready, blocked or proposed, asks to add, approve, finish or drop a task, reports time spent, or when real work worth tracking turns up mid-session. Not for planning days and weeks of sessions (freyja's /planner), and not for repo TODO comments.
---

# Norn

Every task of every project lives in norn: a service on the desktop (`norn.service`, systemd user
unit, `http://127.0.0.1:8080`, no password) over a SQLite database. Read and write it only through
its client, `norn.py` (on the PATH; `norn.py -h`, `norn.py task -h`). `norn.py sql` only reads;
never touch the database file. The code and its book are in `~/repos/norn` (`doc/pages/`).

A task id is `<project>/<area>/<name>`, kebab-case (`baldr/log/plain-default`,
`freyja/gpu/device`). When the author says "the X task", it is in norn, not in a repo TODO.

## Finding

| Question | Command |
|---|---|
| A named task | `norn.py open \| grep -i <word>`, then `norn.py task show <id>` |
| What can start now | `norn.py ready [--project P]` (shelved projects left out) |
| Everything by state | `norn.py board [--project P]` |
| Waiting for approval | `norn.py proposed` |
| What a task waits on | `norn.py chain <id>`, nearest first |
| Its past | `norn.py history --task <id>` |

A list line reads: priority (1 high, 3 low), `✦` if Claude added it, id, kind (`debt`, `feature`,
`review`, `buffer`), state (`proposed`, `ready`, `blocked`, `active`, `done`, `dropped`), estimate
(`original/current` once re-estimated), token estimate, time spent, what it waits on, title.
`--json` gives the API's answer, including the note and refs.

If several tasks match, or none does, say so and ask; don't pick one silently.

## Working on a task

1. **Read it**: `norn.py --json task show <id>`. The note and refs say why it exists and where to
   look. If it is `blocked`, report what it waits on (`chain`) before starting.
2. **Link the session** as soon as work starts: `norn.py task session <id>` (takes
   `$CLAUDE_CODE_SESSION_ID`). Heimdall reads the linked transcripts for the tokens spent. Link
   every task worked on in the session, including ones finished in passing. A task the session
   only added or referred to gets `norn.py task session <id> --mention`: it stays findable from
   the session, but the tokens don't count toward it. Linking again sets the role, which fixes a
   wrong link; `norn.py task sessions <id>` lists them.
3. **Learned it is bigger or smaller**: `norn.py task reestimate <id> <hours> "<why>"`. The
   original estimate never changes; that is the point of the books.
4. **Done**: `norn.py task done <id> "<note>"` once the author has the result (committed, or
   accepted when there is no commit). Abandoned or superseded:
   `norn.py task drop <id> "<why>"`.
5. **Time**: `norn.py log <id> <hours> "<note>" [--date D]` only for hours the author reported or
   confirmed: `log` is human time. Claude's own wall and API time come from the transcripts of
   the work sessions (Heimdall, `heimdall tasks <id>`), so never log them; work done on its own
   may close with no human time at all. When closing a task, ask for the time if it matters to them; offer the session's or
   the day's commit times as a hint, never as the answer.

## Adding tasks

`norn.py task add <id> <kind> <hours> "<title>" [--note N] [--refs R] [--needs A,B]
[--priority 1-3] [--tokens 150k] [--source author|claude]`

- **Source.** A task the author asked for is `author` (the default), even when Claude types it.
  A task Claude noticed on its own is `--source claude`: it stays `proposed`, out of `ready`,
  until the author runs `norn.py task approve <id>`. Claude may add these freely, but only for
  real work items. Anything found outdated, out of sync or broken along the way (a stale skill,
  doc, path or script) gets one without asking; mention it in the reply. No task for an instant
  change done in the session; just do it.
- **Estimate** in quarter hours, including build, tests, docs and the author's review. One task
  is at most one session (4h); split larger work into tasks that each end in a commit.
- **Tokens**: estimate what Claude will spend, cache reads excluded. Calibration: a 2h feature
  session spent about 300k tokens.
- **Priority** 2 unless the author says otherwise or it unblocks something urgent.
- **Note** says why the task exists and what was decided; **refs** point at files, plans or
  commits (`file:line`, `~/repos/plans/...`).
- **Dependencies**: `--needs` when adding, `norn.py dep add <task> <needs>` later. norn refuses
  a cycle.
- Edit later with `norn.py task edit <id> --title/--note/--refs/--priority/--tokens`; delete a
  task added by mistake with `norn.py task rm <id>` (refused once time is logged; drop it then).

After adding, link the session with `--mention` (unless work on the task starts here), show the
author the `task_line` norn printed, and say which ones wait for approval.

## Projects

`norn.py project list`; `project shelve|unshelve <id>` keeps a paused project out of `ready`. A
project appears with its first task. `project owner <id> author|autopilot` sets who decides its
work: in an autopilot project the tasks Claude adds are `ready` at once, with no approval (handing
it back makes them `proposed` again). Ownership is the author's call, like shelving.

## Rules

- When norn refuses a change (an hour that is not a quarter, a cycle, an original estimate, a
  closed task), report its reason; don't work around it.
- Never log time, approve a task, shelve a project or change its owner on your own.
- `norn.py export ~/repos/plans/norn.sql` writes a copy on request; a daily export already goes
  to `/mnt/data/backup/norn`.
- Service down (`cannot reach`): `systemctl --user status norn`; restart it only if the author
  agrees. Updating the service: `deploy/install-systemd.sh` in `~/repos/norn`, then restart.
