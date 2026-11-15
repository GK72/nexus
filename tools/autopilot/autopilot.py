#!/usr/bin/env python3
#
# Part of Nexus Library
#
# Autopilot: works through approved norn tasks on its own, within a plan-usage
# budget, and surveys the repositories for structure and style debt.
#
# The loop is this script, not a model: it reads the budget, picks the next unit
# of work and starts one fresh headless Claude session per unit, so no context
# piles up across tasks and the loop itself costs no tokens.
#
#   autopilot.py                 run until the budget line or nothing is left
#   autopilot.py --dry-run       budget, threshold and the next pick; no tokens
#   autopilot.py --max-runs 1    one unit of work, then stop
#   autopilot.py --retry ID      allow a task an earlier run left behind
#   autopilot.py --survey P      start with a survey of project P
#   autopilot.py report          the last run's report
#
# Units of work:
#   survey  ap-surveyor reads one repository plus consistency.py's findings and
#           proposes debt tasks (--source claude: they wait for the author).
#   task    ap-worker implements one *ready* task on auto/<task> in a worktree
#           and commits; this script rebases it, runs the project's gate and
#           fast-forwards the base branch locally (never pushes), then closes
#           the task. Red gate, conflict or a dirty base checkout: the branch
#           stays and the report says why.
#
# Nova: tasks of the `nova` project land on Nova's `autopilot` branch, never on
# main, and each landing exports nova/<version>@chris/autopilot to Conan.
#
# Budget: stops before a unit when the 5-hour use is over a line that rises from
# `five_hour_start` (window start) to `five_hour_end` (at reset): unused quota
# near reset is lost anyway. Also stops at `seven_day`. Use comes from the
# sessions' own rate_limit events; a running session is cut at line + 10.
#
# Config: ~/.config/nexus/autopilot.toml.
# State:  ~/.local/state/autopilot (state.json, runs.jsonl, report.md);
#         logs and worktrees in ~/.cache/autopilot.

import argparse
import copy
import fcntl
import fnmatch
import json
import os
import signal
import subprocess
import sys
import time
import tomllib
from dataclasses import dataclass, field
from pathlib import Path

HERE = Path(__file__).resolve().parent
NEXUS = HERE.parents[1]
USER_CONFIG = Path.home() / '.config' / 'nexus' / 'autopilot.toml'
STATE_DIR = Path.home() / '.local' / 'state' / 'autopilot'
CACHE_DIR = Path.home() / '.cache' / 'autopilot'
USAGE_FILE = Path.home() / '.claude' / 'usage.json'
WINDOW = 5 * 3600
ELIGIBLE_KINDS = ('debt', 'feature')


# ---------------------------------------------------------------- policy (pure)

@dataclass
class Usage:
    five: float = 0.0           # percent
    five_resets: float = 0.0    # epoch seconds
    week: float = 0.0
    week_resets: float = 0.0


def five_hour_line(now: float, resets_at: float, start: float, end: float) -> float:
    """The allowed 5-hour use: `start` at the window's start, rising to `end` at its reset."""
    left = max(0.0, min(WINDOW, resets_at - now))
    return start + (end - start) * (1 - left / WINDOW)


def budget_check(usage: Usage, now: float, budget: dict) -> tuple[bool, str]:
    five = usage.five if usage.five_resets > now else 0.0
    week = usage.week if usage.week_resets > now else 0.0
    line = five_hour_line(now, usage.five_resets if usage.five_resets > now else now + WINDOW,
                          budget['five_hour_start'], budget['five_hour_end'])
    text = f'5h {five:.0f}% of {line:.0f}%, week {week:.0f}% of {budget["seven_day"]:.0f}%'
    if five >= line:
        return False, f'5-hour budget reached ({text})'
    if week >= budget['seven_day']:
        return False, f'weekly budget reached ({text})'
    return True, text


def over_hard_cap(usage: Usage, now: float, budget: dict) -> bool:
    line = five_hour_line(now, usage.five_resets, budget['five_hour_start'], budget['five_hour_end'])
    return usage.five >= min(95.0, line + 10) or usage.week >= min(95.0, budget['seven_day'] + 5)


@dataclass
class PickContext:
    projects: set[str]
    exclude: list[str]
    attempted: set[str]
    dependents: dict[str, int]          # task -> open tasks waiting on it
    recent_kinds: list[str]             # kinds of the last landed/attempted tasks, newest last
    debt_share: float
    findings: dict[str, int] = field(default_factory=dict)   # project -> consistency errors
    headroom: float = 100.0             # percent points left under the 5-hour line


def score(task: dict, ctx: PickContext) -> float:
    """Priority first; then unblocking, the debt/feature balance, repo debt and size."""
    value = {1: 30, 2: 20, 3: 10}.get(task.get('priority') or 2, 20)
    value += 4 * min(ctx.dependents.get(task['id'], 0), 3)
    recent = ctx.recent_kinds[-10:]
    want_debt = (sum(k == 'debt' for k in recent) / len(recent) if recent else 0.0) < ctx.debt_share
    if (task['kind'] == 'debt') == want_debt:
        value += 8
    if task['kind'] == 'debt':
        value += min(ctx.findings.get(task['project'], 0), 5)
    hours = task.get('current') or 1.0
    if ctx.headroom < 10:
        value -= 3 * hours
    return value


def eligible(task: dict, ctx: PickContext) -> bool:
    # norn's `ready` already means approved, or Claude's task in a project autopilot owns.
    return (task['state'] == 'ready' and task['kind'] in ELIGIBLE_KINDS
            and task['project'] in ctx.projects and task['id'] not in ctx.attempted
            and not any(fnmatch.fnmatch(task['id'], pattern) for pattern in ctx.exclude))


def pick(tasks: list[dict], ctx: PickContext) -> dict | None:
    candidates = [t for t in tasks if eligible(t, ctx)]
    return max(candidates, key=lambda t: (score(t, ctx), t['id']), default=None)


def surveyable(config_projects: dict, norn_projects: dict) -> list[str]:
    """Configured projects open to surveys: not `survey = false`, not shelved in norn."""
    return [p for p, c in config_projects.items()
            if c.get('survey', True) and not norn_projects.get(p, {}).get('shelved', False)]


def survey_due(projects: list[str], surveys: dict[str, float], now: float, interval_days: float) -> str | None:
    due = [p for p in projects if now - surveys.get(p, 0) >= interval_days * 86400]
    return min(due, key=lambda p: surveys.get(p, 0), default=None)


def dependents(deps: list[dict]) -> dict[str, int]:
    counts: dict[str, int] = {}
    for dep in deps:
        counts[dep['needs']] = counts.get(dep['needs'], 0) + 1
    return counts


# ---------------------------------------------------------------- I/O helpers

# Built-in defaults. The config file adds the projects, which describe one
# machine, and may override any of these key by key.
DEFAULTS = {
    'exclude': [],
    'budget': dict(five_hour_start=50, five_hour_end=90, seven_day=80, debt_share=0.4,
                   survey_interval_days=7, tasks_per_survey=3),
    'models': dict(feature='opus', debt='sonnet', survey='sonnet'),
    'tokens': dict(feature=300_000, debt=150_000, survey=120_000),
    'claude': dict(permission_mode='auto', max_turns=200, wall_minutes=90, gate_minutes=45),
}
PROJECT_KEYS = {'repo', 'branch', 'gate', 'survey', 'from', 'worktree'}


def merge_config(user: dict) -> dict:
    """The defaults overlaid by the parsed config file; raises ValueError on a bad file."""
    config = copy.deepcopy(DEFAULTS)
    for key, value in user.items():
        if key == 'projects':
            continue
        if key not in DEFAULTS:
            raise ValueError(f'unknown key `{key}`')
        if isinstance(DEFAULTS[key], dict):
            unknown = set(value) - set(DEFAULTS[key])
            if unknown:
                raise ValueError(f'unknown key in [{key}]: {", ".join(sorted(unknown))}')
            config[key].update(value)
        else:
            config[key] = value
    projects = user.get('projects') or {}
    if not projects:
        raise ValueError('no [projects.<id>] tables: nothing to work on')
    for project_id, project in projects.items():
        if missing := {'repo', 'branch'} - set(project):
            raise ValueError(f'[projects.{project_id}] lacks {", ".join(sorted(missing))}')
        if unknown := set(project) - PROJECT_KEYS:
            raise ValueError(f'unknown key in [projects.{project_id}]: {", ".join(sorted(unknown))}')
        if 'worktree' in project and 'from' not in project:
            raise ValueError(f'[projects.{project_id}] has a worktree but no `from` branch to create it from')
    config['projects'] = copy.deepcopy(projects)
    return config


def load_config(path: Path | None) -> dict:
    file = path or USER_CONFIG
    if not file.is_file():
        sys.exit(f'autopilot: no config at {file}; doc/autopilot, Configuration, shows the format')
    try:
        config = merge_config(tomllib.loads(file.read_text()))
    except (ValueError, tomllib.TOMLDecodeError) as error:
        sys.exit(f'autopilot: {file}: {error}')
    for project in config['projects'].values():
        for key in ('repo', 'worktree'):
            if key in project:
                project[key] = str(Path(project[key]).expanduser())
    return config


def sh(args: list[str], cwd: str | Path | None = None, check: bool = True, timeout: float | None = None) -> str:
    result = subprocess.run(args, cwd=cwd, capture_output=True, text=True, timeout=timeout)
    if check and result.returncode != 0:
        raise RuntimeError(f'{" ".join(args)}: {result.stderr.strip() or result.stdout.strip()}')
    return result.stdout.strip()


def git(repo: str | Path, *args: str, check: bool = True) -> str:
    return sh(['git', '-C', str(repo), *args], check=check)


def norn(*args: str) -> str:
    return sh(['norn.py', *args])


def norn_json(*args: str):
    return json.loads(norn('--json', *args))


class State:
    def __init__(self) -> None:
        self.path = STATE_DIR / 'state.json'
        self.data = json.loads(self.path.read_text()) if self.path.is_file() else {}
        self.data.setdefault('surveys', {})
        self.data.setdefault('attempts', {})
        self.data.setdefault('kinds', [])
        self.data.setdefault('findings', {})

    def save(self) -> None:
        tmp = self.path.with_suffix('.tmp')
        tmp.write_text(json.dumps(self.data, indent=1))
        tmp.replace(self.path)


def log_run(record: dict) -> None:
    with (STATE_DIR / 'runs.jsonl').open('a') as out:
        out.write(json.dumps(record) + '\n')


def usage_from_event(info: dict, usage: Usage) -> None:
    windows = info.get('unifiedWindows') or {}
    if 'five_hour' in windows:
        usage.five = 100 * windows['five_hour']['utilization']
        usage.five_resets = windows['five_hour']['resetsAt']
    if 'seven_day' in windows:
        usage.week = 100 * windows['seven_day']['utilization']
        usage.week_resets = windows['seven_day']['resetsAt']


# ---------------------------------------------------------------- claude sessions

@dataclass
class Session:
    session_id: str = ''
    result: str = ''
    tokens: int = 0         # input + cache writes + output; cache reads left out
    cut: str = ''           # why the session was stopped early, if it was
    exit_code: int = 0


def run_claude(prompt: str, agent: str, model: str, cwd: Path, log: Path, config: dict,
               usage: Usage, token_cap: int, on_start=None) -> Session:
    args = ['claude', '-p', prompt, '--agent', agent, '--model', model,
            '--permission-mode', config['claude']['permission_mode'],
            '--max-turns', str(config['claude']['max_turns']),
            '--output-format', 'stream-json', '--verbose', '--strict-mcp-config']
    session = Session()
    seen: set[str] = set()
    deadline = time.time() + config['claude']['wall_minutes'] * 60
    with log.open('w') as raw, subprocess.Popen(args, cwd=cwd, stdout=subprocess.PIPE, text=True,
                                                start_new_session=True) as proc:
        assert proc.stdout
        for line in proc.stdout:
            raw.write(line)
            try:
                event = json.loads(line)
            except json.JSONDecodeError:
                continue
            kind = event.get('type')
            if kind == 'system' and event.get('subtype') == 'init':
                session.session_id = event.get('session_id', '')
                if on_start:
                    on_start(session.session_id)
            elif kind == 'rate_limit_event':
                usage_from_event(event.get('rate_limit_info') or {}, usage)
            elif kind == 'assistant':
                message = event.get('message', {})
                if message.get('id') not in seen:
                    seen.add(message.get('id'))
                    used = message.get('usage', {})
                    session.tokens += (used.get('input_tokens', 0) + used.get('output_tokens', 0)
                                       + used.get('cache_creation_input_tokens', 0))
            elif kind == 'result':
                session.result = event.get('result') or ''
            if not session.cut:
                if over_hard_cap(usage, time.time(), config['budget']):
                    session.cut = 'hard budget cap'
                elif session.tokens > token_cap:
                    session.cut = f'token cap {token_cap // 1000}k'
                elif time.time() > deadline:
                    session.cut = 'wall-clock cap'
                if session.cut:
                    os.killpg(proc.pid, signal.SIGTERM)
        session.exit_code = proc.wait()
    return session


def probe_usage(config: dict) -> Usage:
    usage = Usage()
    if USAGE_FILE.is_file():
        data = json.loads(USAGE_FILE.read_text())
        if time.time() - data.get('updated', 0) < 300:
            limits = data.get('rate_limits') or {}
            usage.five = limits.get('five_hour', {}).get('used_percentage', 0)
            usage.five_resets = limits.get('five_hour', {}).get('resets_at', 0)
            usage.week = limits.get('seven_day', {}).get('used_percentage', 0)
            usage.week_resets = limits.get('seven_day', {}).get('resets_at', 0)
            return usage
    out = sh(['claude', '-p', 'reply ok', '--model', 'haiku', '--max-turns', '1', '--strict-mcp-config',
              '--output-format', 'stream-json', '--verbose'], cwd=CACHE_DIR, check=False, timeout=120)
    for line in out.splitlines():
        if '"rate_limit_event"' in line:
            usage_from_event(json.loads(line).get('rate_limit_info') or {}, usage)
    return usage


def result_line(text: str) -> str:
    for line in reversed(text.strip().splitlines()):
        if line.startswith('RESULT:'):
            return line[len('RESULT:'):].strip()
    return (text.strip().splitlines() or ['no result'])[-1][:200]


# ---------------------------------------------------------------- git landing

def slug(task_id: str) -> str:
    return task_id.replace('/', '-')


def checkout_of(repo: str, branch: str) -> str | None:
    """The worktree that has `branch` checked out, if any."""
    current = None
    for line in git(repo, 'worktree', 'list', '--porcelain').splitlines():
        if line.startswith('worktree '):
            current = line.split(' ', 1)[1]
        elif line == f'branch refs/heads/{branch}':
            return current
    return None


def fast_forward(repo: str, base: str, branch: str) -> str:
    """Moves `base` to `branch` without touching anyone's work. Returns '' or why not."""
    if subprocess.run(['git', '-C', repo, 'merge-base', '--is-ancestor', base, branch]).returncode != 0:
        return f'{base} moved on; rebase needed'
    holder = checkout_of(repo, base)
    if holder is None:
        git(repo, 'update-ref', f'refs/heads/{base}', git(repo, 'rev-parse', branch), git(repo, 'rev-parse', base))
        return ''
    if git(holder, 'status', '--porcelain', '--untracked-files=no'):
        return f'{base} is checked out with uncommitted changes in {holder}'
    result = subprocess.run(['git', '-C', holder, 'merge', '--ff-only', '--quiet', branch],
                            capture_output=True, text=True)
    return '' if result.returncode == 0 else f'fast-forward failed: {result.stderr.strip()[:200]}'


def ensure_worktree(project: dict) -> None:
    """Nova's persistent `autopilot` worktree, created from main on first use."""
    path = project.get('worktree')
    if not path or Path(path).exists():
        return
    repo, branch = project['repo'], project['branch']
    exists = subprocess.run(['git', '-C', repo, 'rev-parse', '--verify', '--quiet', branch]).returncode == 0
    git(repo, 'worktree', 'add', *([path, branch] if exists else ['-b', branch, path, project['from']]))
    exclude = Path(git(repo, 'rev-parse', '--git-common-dir')) / 'info' / 'exclude'
    if not exclude.is_absolute():
        exclude = Path(repo) / exclude
    if '.claude/' not in (exclude.read_text() if exclude.is_file() else ''):
        with exclude.open('a') as out:
            out.write('.claude/\n')


def gate_command(project: dict, path: Path) -> str:
    if project.get('gate'):
        return project['gate'].replace('{base}', project['branch'])
    if (path / 'ci' / 'pipeline.sh').is_file():
        return f'ci/pipeline.sh --changed --base {project["branch"]}'
    return 'baldr build -b Debug && ctest --test-dir build/debug --output-on-failure'


# ---------------------------------------------------------------- units of work

class Autopilot:
    def __init__(self, config: dict, args) -> None:
        self.config = config
        self.args = args
        self.state = State()
        self.usage = Usage()
        self.report: list[str] = []
        self.started = time.time()

    # -- choosing

    def context(self, tasks: list[dict]) -> PickContext:
        deps = norn_json('graph', '--format', 'json') if tasks else {'deps': []}
        attempted = set(self.state.data['attempts']) - set(self.args.retry or [])
        line = five_hour_line(time.time(), self.usage.five_resets, self.config['budget']['five_hour_start'],
                              self.config['budget']['five_hour_end'])
        return PickContext(projects=set(self.config['projects']), exclude=self.config.get('exclude', []),
                           attempted=attempted, dependents=dependents(deps.get('deps', [])),
                           recent_kinds=self.state.data['kinds'], debt_share=self.config['budget']['debt_share'],
                           findings=self.state.data['findings'], headroom=max(0.0, line - self.usage.five))

    def next_unit(self, tasks_since_survey: int) -> tuple[str, object] | None:
        tasks = [t for t in norn_json('ready') if t['project'] in self.config['projects']]
        task = pick(tasks, self.context(tasks))
        candidates = surveyable(self.config['projects'], self.norn_projects())
        due = survey_due(candidates, self.state.data['surveys'], time.time(),
                         self.config['budget']['survey_interval_days'])
        if due and (task is None or tasks_since_survey >= self.config['budget']['tasks_per_survey']):
            return 'survey', due
        return ('task', task) if task else None

    # -- survey

    def survey(self, project_id: str) -> dict:
        project = self.config['projects'][project_id]
        repo = project.get('worktree') or project['repo']
        checker = subprocess.run([sys.executable, str(HERE / 'consistency.py'), repo], capture_output=True, text=True)
        findings = checker.stdout.strip()
        self.state.data['findings'][project_id] = findings.count('error ')
        owner = self.norn_projects().get(project_id, {}).get('owner', 'author')
        open_tasks = norn('open', '--project', project_id) if project_id in self.norn_projects() else '(none yet)'
        nova = self.config['projects'].get('nova')
        headers = ' '.join(sorted(p.stem for p in (Path(nova['repo']) / 'libnova' / 'libnova').glob('*.hpp'))) \
            if nova else '(no nova project configured)'
        prompt = (f'Survey project `{project_id}`, repository {repo} (base branch {project["branch"]}).\n'
                  f'Owner: {owner}.\n\n'
                  f'consistency.py findings (against the template):\n{findings}\n\n'
                  f'Open norn tasks of the project (do not duplicate):\n{open_tasks}\n\n'
                  f'Nova headers (libnova/libnova/*.hpp): {headers}\n')
        log = CACHE_DIR / 'logs' / f'{time.strftime("%Y%m%d-%H%M%S")}-survey-{project_id}.jsonl'
        session = run_claude(prompt, 'ap-surveyor', self.config['models']['survey'], Path(repo), log,
                             self.config, self.usage, self.config['tokens']['survey'])
        self.state.data['surveys'][project_id] = time.time()
        return dict(unit='survey', project=project_id, session=session.session_id, tokens=session.tokens,
                    cut=session.cut, result=result_line(session.result), log=str(log))

    def norn_projects(self) -> dict[str, dict]:
        """norn's projects by id: `shelved`, `owner`."""
        return {p['id']: p for p in norn_json('project', 'list')}

    # -- task

    def work(self, task: dict) -> dict:
        project = self.config['projects'][task['project']]
        ensure_worktree(project)
        repo, base = project['repo'], project['branch']
        branch = f'auto/{task["id"]}'
        path = CACHE_DIR / 'wt' / slug(task['id'])
        if subprocess.run(['git', '-C', repo, 'rev-parse', '--verify', '--quiet', branch],
                          capture_output=True).returncode == 0:
            git(repo, 'branch', '-m', branch, f'{branch}.{time.strftime("%Y%m%d-%H%M%S")}')
        if path.exists():
            git(repo, 'worktree', 'remove', '--force', str(path))
        git(repo, 'worktree', 'add', '-b', branch, str(path), base)

        gate = gate_command(project, path)
        full = norn_json('task', 'show', task['id'])
        tokens = task.get('tokens') or self.config['tokens'][task['kind']]
        prompt = '\n'.join([
            f'Task (norn): {task["id"]} [{task["kind"]}, estimate {task.get("current")}h, ~{tokens // 1000}k tokens]',
            f'Title: {task["title"]}',
            f'Note: {full.get("note") or "-"}',
            f'Refs: {full.get("refs") or "-"}',
            f'Worktree: {path} (branch {branch}, from {base} of {repo})',
            f'Gate (autopilot runs it after you; it must pass): {gate}',
            *([self.nova_context()] if task['project'] != 'nova' else
              ['This is Nova: you are on a branch of `autopilot`, never main. Bump libnova/version.txt one minor '
               'above main\'s if it still equals it, and add a CHANGELOG.md entry.']),
        ])
        log = CACHE_DIR / 'logs' / f'{time.strftime("%Y%m%d-%H%M%S")}-{slug(task["id"])}.jsonl'
        model = self.config['models'][task['kind']]
        session = run_claude(prompt, 'ap-worker', model, path, log, self.config, self.usage, 2 * tokens,
                             on_start=lambda sid: norn('task', 'session', task['id'], sid))
        record = dict(unit='task', task=task['id'], kind=task['kind'], model=model, session=session.session_id,
                      tokens=session.tokens, cut=session.cut, result=result_line(session.result),
                      branch=branch, log=str(log))
        record['outcome'] = self.land(task, project, path, branch, gate, session)
        if path.exists():       # not landed: the branch keeps the work, the worktree (and its build) goes
            self.drop_worktree(repo, path, branch, delete_branch=False)
        return record

    def land(self, task: dict, project: dict, path: Path, branch: str, gate: str, session: Session) -> str:
        repo, base = project['repo'], project['branch']
        if git(path, 'status', '--porcelain'):
            git(path, 'stash', '--include-untracked', '--quiet', check=False)
        if git(repo, 'rev-list', '--count', f'{base}..{branch}') == '0':
            self.drop_worktree(repo, path, branch, delete_branch=True)
            return f'no commits ({session.cut or "agent stopped"})'
        if session.cut:
            return f'cut ({session.cut}); work left on {branch}'
        if subprocess.run(['git', '-C', str(path), 'rebase', '--quiet', base], capture_output=True).returncode != 0:
            git(path, 'rebase', '--abort', check=False)
            return f'conflicts with {base}; left on {branch}'
        gate_log = CACHE_DIR / 'logs' / f'{time.strftime("%Y%m%d-%H%M%S")}-{slug(task["id"])}-gate.log'
        try:
            with gate_log.open('w') as out:
                code = subprocess.run(['bash', '-c', gate], cwd=path, stdout=out, stderr=subprocess.STDOUT,
                                      timeout=self.config['claude']['gate_minutes'] * 60).returncode
        except subprocess.TimeoutExpired:
            code = -1
        if code != 0:
            return f'gate failed ({gate_log}); left on {branch}'
        why = fast_forward(repo, base, branch)
        if why:
            return f'{why}; green on {branch}'
        sha = git(repo, 'rev-parse', '--short', base)
        norn('task', 'done', task['id'], f'autopilot: {sha} on {base} (local, not pushed)')
        self.drop_worktree(repo, path, branch, delete_branch=True)
        if task['project'] == 'nova':
            return f'landed {sha} on {base}; {self.export_nova(project)}'
        return f'landed {sha} on {base}'

    def drop_worktree(self, repo: str, path: Path, branch: str, delete_branch: bool) -> None:
        git(repo, 'worktree', 'remove', '--force', str(path), check=False)
        if delete_branch:
            git(repo, 'branch', '-D', branch, check=False)

    def nova_context(self) -> str:
        nova = self.config['projects'].get('nova')
        if not nova:
            return 'Nova: released versions only.'
        path = Path(nova['worktree'])
        if not path.is_dir():
            return f'Nova: released nova/<version> from {nova["repo"]} main; no autopilot build of Nova exists yet.'
        version = (path / 'libnova' / 'version.txt').read_text().strip()
        return (f'Nova: released versions from main; the autopilot build nova/{version}@chris/autopilot '
                f'(worktree {path}, branch autopilot) may be used when a task needs its helpers.')

    def export_nova(self, project: dict) -> str:
        path = Path(project['worktree'])
        version = (path / 'libnova' / 'version.txt').read_text().strip()
        result = subprocess.run(['conan', 'create', str(path / 'libnova'), '--user', 'chris', '--channel', 'autopilot',
                                 '--test-folder=', '--build=missing', '--settings', 'compiler.cppstd=20'],
                                capture_output=True, text=True)
        return (f'exported nova/{version}@chris/autopilot' if result.returncode == 0
                else f'conan export FAILED: {result.stderr.strip()[-200:]}')

    # -- loop

    def run(self) -> int:
        self.usage = probe_usage(self.config)
        tasks_since_survey = self.config['budget']['tasks_per_survey']   # a due survey goes first
        runs = 0
        reason = ''
        while True:
            ok, text = budget_check(self.usage, time.time(), self.config['budget'])
            if not ok:
                reason = text
                break
            unit = ('survey', self.args.survey) if self.args.survey and not runs else self.next_unit(tasks_since_survey)
            if unit is None:
                reason = f'nothing left to do ({text})'
                break
            kind, what = unit
            label = what if kind == 'survey' else what['id']
            print(f'[{time.strftime("%H:%M")}] {text} -> {kind} {label}', flush=True)
            if self.args.dry_run:
                reason = 'dry run'
                if kind == 'task':
                    print(f'  {what["kind"]}, priority {what.get("priority")}, {what.get("current")}h: {what["title"]}')
                break
            try:
                record = self.survey(what) if kind == 'survey' else self.work(what)
            except Exception as error:     # one broken unit must not end a long unattended run
                record = dict(unit=kind, task=label, outcome=f'error: {error}')
            record['at'] = time.strftime('%Y-%m-%d %H:%M')
            if kind == 'task':
                tasks_since_survey += 1
                self.state.data['attempts'][what['id']] = dict(outcome=record.get('outcome'), at=record['at'])
                self.state.data['kinds'] = (self.state.data['kinds'] + [what['kind']])[-50:]
            else:
                tasks_since_survey = 0
            self.state.save()
            log_run(record)
            self.report.append(self.report_line(record))
            print(f'  {self.report[-1]}', flush=True)
            runs += 1
            if self.args.max_runs and runs >= self.args.max_runs:
                reason = f'--max-runs {self.args.max_runs}'
                break
        self.write_report(reason)
        return 0

    @staticmethod
    def report_line(record: dict) -> str:
        what = record.get('task') or record.get('project')
        tokens = f'{record.get("tokens", 0) // 1000}k tok'
        if record['unit'] == 'survey':
            return f'survey {what}: {record.get("result", record.get("outcome"))} [{tokens}]'
        return f'{what}: {record.get("outcome")} -- {record.get("result", "")} [{tokens}]'

    def write_report(self, reason: str) -> None:
        minutes = (time.time() - self.started) / 60
        lines = [f'# Autopilot {time.strftime("%Y-%m-%d %H:%M")}', '',
                 f'Stopped: {reason}. {len(self.report)} units in {minutes:.0f} min.', '',
                 *[f'- {line}' for line in self.report]]
        tagged = [p for p, c in self.config['projects'].items() if p != 'nova' and '@chris/autopilot' in
                  ''.join((Path(c['repo']) / f).read_text() for f in ('conanfile.py', 'conandata.yml')
                          if (Path(c['repo']) / f).is_file())]
        if tagged:
            lines += ['', f'Built on the unreviewed Nova autopilot build: {", ".join(tagged)}']
        proposed = norn('proposed')
        if proposed:
            lines += ['', 'Awaiting your approval (`norn.py task approve <id>`):', '', proposed]
        text = '\n'.join(lines) + '\n'
        if not self.args.dry_run:
            (STATE_DIR / 'report.md').write_text(text)
        print('\n' + text)


def main() -> int:
    parser = argparse.ArgumentParser(description='Autonomous work on norn tasks within a usage budget.')
    parser.add_argument('command', nargs='?', choices=['run', 'report'], default='run')
    parser.add_argument('--dry-run', action='store_true', help='show the budget and the next unit; no tokens')
    parser.add_argument('--max-runs', type=int, default=0, help='stop after this many units')
    parser.add_argument('--retry', action='append', metavar='ID', help='allow a task an earlier run left behind')
    parser.add_argument('--survey', metavar='PROJECT', help='start with a survey of this project')
    parser.add_argument('--config', type=Path, help=f'override file (default {USER_CONFIG})')
    args = parser.parse_args()

    for directory in (STATE_DIR, CACHE_DIR / 'logs', CACHE_DIR / 'wt'):
        directory.mkdir(parents=True, exist_ok=True)
    if args.command == 'report':
        report = STATE_DIR / 'report.md'
        print(report.read_text() if report.is_file() else 'no report yet')
        return 0

    with (STATE_DIR / 'lock').open('w') as lock:
        try:
            fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError:
            print('autopilot: another run is active', file=sys.stderr)
            return 1
        return Autopilot(load_config(args.config), args).run()


if __name__ == '__main__':
    sys.exit(main())
