#!/usr/bin/env python3
"""Sanity-check C++ file headers and report code maturity.

Expected header (the block comment must be the first thing in the file):

    /**
     * Part of <project>
     *
     * @brief   One-sentence summary, which may continue on further lines,
     *          aligned with its first line.
     *
     * Optional details.
     *
     * @author      Name
     * @coauthor    Name                    (optional, repeatable)
     * @created     2026-04-11
     * @updated     2026-11-06
     * @status      evolving
     * @reviewed    skimmed
     * SPDX-License-Identifier: Unlicense
     *
     * Optional free text.
     */

@status:   experimental, evolving, stable, frozen, deprecated <replacement>
@reviewed: none, skimmed, read, audited

A `.hpp` (else `.h`) is the authoritative file of its module: a sibling `.cpp`
must agree with it. A `*.test.cpp` inherits @status from the module it covers
and keeps its own @reviewed; a test without a module carries its own @status.
Files matching `no_status` in the configuration, e.g., examples, carry no @status.

Errors are broken format, invalid values and unmet status requirements.
Warnings are judgement calls (stale reviews, churn, risky includes) and never
fail the run.

--check and --report only read. --fix rewrites headers: it does the mechanical
work and fills in what is missing, but never raises @status or @reviewed. New
values default to `experimental` and `none`, and an existing value it doesn't
recognise is left for a human. It never invents a description: a file without
one gets an empty @brief, which --check keeps reporting. Tests of a module get
a generated one. @updated moves to today when the code differs from HEAD, so a
header-only change doesn't count. Everything after the header stays untouched.

--pre-commit is what the hook runs, and --install-hook sets it up for a
repository. On the staged content, it moves @updated of files whose code changed
and checks the staged files: errors stop the commit, warnings don't. Changes that
aren't staged stay out of the commit. Merges and the like are left alone, as is
the date of a new file, which may have moved here from another repository.

After `git commit <path>`, git's real index keeps the old date until the next
`git add`, because git locks it during such a commit. The commit itself is right.

Set FILE_HEADERS_NO_BUMP=1 to keep the dates, e.g., for a bulk reformat, and use
`git commit --no-verify` to skip the hook altogether.

Configuration lives in `.file-headers.toml` at the repository root.
"""

import argparse
import fnmatch
import os
import re
import shlex
import subprocess
import sys
import tomllib
from collections import Counter
from dataclasses import dataclass, field
from datetime import date
from pathlib import Path

CONFIG_NAME = ".file-headers.toml"
HOOK_MARKER = "# Installed by file_headers.py --install-hook"
NO_BUMP_VARIABLE = "FILE_HEADERS_NO_BUMP"

STATUS_RANK = {"experimental": 0, "evolving": 1, "stable": 2, "frozen": 3}
STATUSES = [*STATUS_RANK, "deprecated"]
REVIEW_RANK = {"none": 0, "skimmed": 1, "read": 2, "audited": 3}
FIELD_ORDER = ["author", "coauthor", "created", "updated", "status", "reviewed", "license"]
REPEATABLE = {"coauthor"}

DEFAULT_STATUS = "experimental"
DEFAULT_REVIEW = "none"

STABLE_RANK = STATUS_RANK["stable"]
MIN_REVIEW = {"stable": "read", "frozen": "audited"}
MIN_TEST_REVIEW = "read"

BRIEF_WIDTH = len("@brief   ")
TAG_WIDTH = len("@compliance  ")
LICENSE_TAG = "SPDX-License-Identifier:"

TAG_LINE = re.compile(r"^@(\w+)\s*(.*)$")
LICENSE_LINE = re.compile(rf"^{LICENSE_TAG}\s*(.*)$")
PART_OF_LINE = re.compile(r"^Part of (.+)$")
ISO_DATE = re.compile(r"^\d{4}-\d{2}-\d{2}$")
INCLUDE_LINE = re.compile(r'^\s*#\s*include\s*([<"])([^>"]+)[>"]')
OPEN_WORK = re.compile(r"\b(TODO|FIXME)\b")


@dataclass(frozen=True)
class Finding:
    path: str
    line: int
    severity: str
    message: str


@dataclass(frozen=True)
class Tag:
    name: str
    value: str
    raw: str
    line: int
    paragraph: int


@dataclass
class Header:
    part_of: str | None = None
    part_of_line: int = 1
    part_of_lines: int = 0
    brief: Tag | None = None
    tags: list[Tag] = field(default_factory=list)
    block_paragraph: int | None = None
    block_text_lines: list[int] = field(default_factory=list)
    malformed_lines: list[int] = field(default_factory=list)
    misaligned_brief_lines: list[int] = field(default_factory=list)
    has_description: bool = False

    def value(self, name):
        return next((tag.value for tag in self.tags if tag.name == name), None)

    def tag(self, name):
        return next((tag for tag in self.tags if tag.name == name), None)

    def status_name(self):
        status = self.value("status")
        return status.split()[0] if status else None


@dataclass
class Source:
    path: str
    text: str
    header: Header | None
    is_test: bool = False
    primary: str | None = None


@dataclass(frozen=True)
class Config:
    extensions: tuple = (".h", ".hpp", ".cpp")
    exclude: tuple = ()
    license: str = "Unlicense"
    author: str = ""
    projects: dict = field(default_factory=dict)
    include_roots: tuple = (".",)
    no_test_required: tuple = ()
    no_status: tuple = ()
    stale_lines: int = 100
    fix_commit_pattern: str = r"(?i)\bfix"


def load_config(path):
    with open(path, "rb") as config_file:
        raw = tomllib.load(config_file)

    known = Config.__dataclass_fields__
    unknown = sorted(set(raw) - set(known))
    if unknown:
        raise ValueError(f"unknown config keys: {', '.join(unknown)}")

    return Config(**{key: tuple(value) if isinstance(value, list) else value
                     for key, value in raw.items()})


def comment_lines(text):
    """Returns the (line number, content) pairs of a leading block comment."""
    lines = text.splitlines()
    if not lines or lines[0].rstrip() != "/**":
        return None

    body = []
    for number, line in enumerate(lines[1:], start=2):
        if line.strip() == "*/":
            return body
        body.append((number, line))

    return None


def comment_content(line):
    """Returns the text of a comment line, without its ` * ` prefix."""
    return re.sub(r"^\s*\*? ?", "", line.rstrip())


def parse_header(text):
    """Parses the leading block comment, or returns None if there isn't one."""
    body = comment_lines(text)
    if body is None:
        return None

    header = Header()
    paragraph = 0
    paragraph_has_text = False
    in_brief = False
    first_paragraph = []

    for number, line in body:
        stripped = line.rstrip()
        if stripped != " *" and not stripped.startswith(" * "):
            header.malformed_lines.append(number)

        content = comment_content(stripped)
        if not content:
            paragraph += paragraph_has_text
            paragraph_has_text = False
            in_brief = False
            continue

        paragraph_has_text = True
        if paragraph == 0:
            first_paragraph.append((number, content))

        license_match = LICENSE_LINE.match(content)
        tag_match = TAG_LINE.match(content)
        if license_match or tag_match:
            in_brief = False
            name = "license" if license_match else tag_match.group(1)
            value = (license_match or tag_match).group(1 if license_match else 2)
            tag = Tag(name, value.strip(), content, number, paragraph)
            if name == "brief" and header.brief is None:
                header.brief = tag
                in_brief = True
            else:
                header.tags.append(tag)
                if header.block_paragraph is None:
                    header.block_paragraph = paragraph
        elif in_brief:
            brief = header.brief
            continuation = content.strip()
            if content != " " * BRIEF_WIDTH + continuation:
                header.misaligned_brief_lines.append(number)
            header.brief = Tag(brief.name, f"{brief.value} {continuation}".strip(), brief.raw,
                               brief.line, brief.paragraph)
        elif paragraph == header.block_paragraph:
            header.block_text_lines.append(number)
        elif paragraph > 0:
            header.has_description = True

    if first_paragraph:
        header.part_of_line = first_paragraph[0][0]
        header.part_of_lines = len(first_paragraph)
        match = PART_OF_LINE.match(first_paragraph[0][1])
        header.part_of = match.group(1) if match else None

    return header


def expected_project(path, config):
    """Returns the project of the longest configured prefix matching `path`."""
    prefixes = [prefix for prefix in config.projects
                if path == prefix or path.startswith(prefix.rstrip("/") + "/")]
    return config.projects[max(prefixes, key=len)] if prefixes else None


def check_part_of(path, header, config):
    line = header.part_of_line
    if header.part_of is None:
        return [Finding(path, line, "error", "missing 'Part of <project>' as the first line")]

    findings = []
    if header.part_of_lines > 1:
        findings.append(Finding(path, line, "error", "'Part of <project>' must be a single line"))

    project = expected_project(path, config)
    if project is None:
        findings.append(Finding(path, line, "error", "no project configured for this path"))
    elif header.part_of != project:
        findings.append(Finding(path, line, "error",
                                f"expected 'Part of {project}', found 'Part of {header.part_of}'"))

    return findings


def check_brief(path, header):
    brief = header.brief
    if brief is None:
        reason = "description is untagged" if header.has_description else "no description"
        return [Finding(path, header.part_of_line, "error", f"missing @brief ({reason})")]

    findings = []
    if brief.paragraph != 1:
        findings.append(Finding(path, brief.line, "error",
                                "@brief must follow the 'Part of' line after one blank line"))
    if not brief.value:
        findings.append(Finding(path, brief.line, "error", "@brief is empty"))
    elif brief.raw != f"{'@brief':<{BRIEF_WIDTH}}{brief.raw[len('@brief'):].strip()}":
        findings.append(Finding(path, brief.line, "error",
                                f"misaligned @brief: value must start at column {BRIEF_WIDTH}"))
    findings += [Finding(path, line, "error",
                         f"misaligned @brief: continuation must start at column {BRIEF_WIDTH}")
                 for line in header.misaligned_brief_lines]
    if header.block_paragraph == brief.paragraph:
        findings.append(Finding(path, brief.line, "error",
                                "@brief must be separated from the tags by a blank line"))

    return findings


def check_tag_layout(path, header):
    findings = []
    previous_rank = -1
    seen = Counter()

    for tag in header.tags:
        in_block = tag.paragraph == header.block_paragraph
        if tag.name == "date":
            findings.append(Finding(path, tag.line, "error", "@date is replaced by @created"))
            continue
        if tag.name not in FIELD_ORDER:
            if in_block:
                findings.append(Finding(path, tag.line, "error", f"unknown tag @{tag.name}"))
            continue
        if not in_block:
            findings.append(Finding(path, tag.line, "error",
                                    f"@{tag.name} is outside the tag block"))
            continue

        seen[tag.name] += 1
        if seen[tag.name] > 1 and tag.name not in REPEATABLE:
            findings.append(Finding(path, tag.line, "error", f"duplicate @{tag.name}"))

        rank = FIELD_ORDER.index(tag.name)
        if rank < previous_rank:
            findings.append(Finding(path, tag.line, "error",
                                    f"@{tag.name} out of order: expected "
                                    f"{', '.join(FIELD_ORDER)}"))
        previous_rank = max(previous_rank, rank)

        if tag.name == "license":
            aligned = f"{LICENSE_TAG} {tag.value}"
        else:
            aligned = f"{'@' + tag.name:<{TAG_WIDTH}}{tag.value}"
        if tag.raw != aligned:
            findings.append(Finding(path, tag.line, "error",
                                    f"misaligned @{tag.name}: expected '{aligned}'"))

    findings += [Finding(path, line, "error", "free text inside the tag block")
                 for line in header.block_text_lines]
    return findings


def check_required_tags(path, header, needs_status):
    required = [name for name in FIELD_ORDER if name != "coauthor"]
    if not needs_status:
        required.remove("status")
    if header.tag("date"):
        required.remove("created")

    return [Finding(path, 1, "error", f"missing @{name}")
            for name in required if header.tag(name) is None]


def check_tag_values(path, header, config, root):
    findings = []

    author = header.tag("author")
    if author and not author.value:
        findings.append(Finding(path, author.line, "error", "@author is empty"))
    elif author and " and " in author.value:
        findings.append(Finding(path, author.line, "error",
                                "one person per @author: use @coauthor for the others"))

    parsed_dates = {}
    for name in ("created", "updated"):
        tag = header.tag(name)
        if tag is None:
            continue
        try:
            if not ISO_DATE.match(tag.value):
                raise ValueError
            parsed_dates[name] = date.fromisoformat(tag.value)
        except ValueError:
            findings.append(Finding(path, tag.line, "error",
                                    f"@{name} is not a YYYY-MM-DD date: '{tag.value}'"))

    if len(parsed_dates) == 2 and parsed_dates["created"] > parsed_dates["updated"]:
        findings.append(Finding(path, header.tag("created").line, "error",
                                "@created is later than @updated"))

    status = header.tag("status")
    if status:
        findings += check_status_value(path, status, root)

    reviewed = header.tag("reviewed")
    if reviewed and reviewed.value not in REVIEW_RANK:
        findings.append(Finding(path, reviewed.line, "error",
                                f"unknown @reviewed '{reviewed.value}': "
                                f"expected one of {', '.join(REVIEW_RANK)}"))

    license_tag = header.tag("license")
    if license_tag and license_tag.value != config.license:
        findings.append(Finding(path, license_tag.line, "error",
                                f"expected license {config.license}, found '{license_tag.value}'"))

    return findings


def check_status_value(path, status, root):
    words = status.value.split()
    name = words[0] if words else ""

    if name not in STATUSES:
        return [Finding(path, status.line, "error",
                        f"unknown @status '{status.value}': "
                        f"expected one of {', '.join(STATUSES)}")]
    if name != "deprecated":
        if len(words) > 1:
            return [Finding(path, status.line, "error", f"@status {name} takes no argument")]
        return []
    if len(words) != 2:
        return [Finding(path, status.line, "error",
                        "@status deprecated needs exactly one replacement path")]
    if not (Path(root) / words[1]).exists():
        return [Finding(path, status.line, "error", f"replacement '{words[1]}' does not exist")]
    return []


def carries_status(path, config):
    return not any(fnmatch.fnmatch(path, pattern) for pattern in config.no_status)


def check_format(source, sources, config, root):
    """All uniform checks that apply to a single file, at every status."""
    path = source.path
    header = source.header
    if header is None:
        return [Finding(path, 1, "error", "missing file header")]

    inherits_status = source.is_test and source.primary is not None
    exempt = not carries_status(path, config)
    findings = [Finding(path, line, "error", "comment line must start with ' * '")
                for line in header.malformed_lines]
    findings += check_part_of(path, header, config)
    findings += check_brief(path, header)
    findings += check_tag_layout(path, header)
    findings += check_required_tags(path, header, not (inherits_status or exempt))
    findings += check_tag_values(path, header, config, root)

    status = header.tag("status")
    if inherits_status and status:
        findings.append(Finding(path, status.line, "error",
                                f"tests inherit @status from {source.primary}: remove it"))
    elif exempt and status:
        findings.append(Finding(path, status.line, "error", "no @status expected here: remove it"))

    return findings


def split_module(path):
    """Returns (stem, is_test) of a source path."""
    stem, _ = os.path.splitext(path)
    if stem.endswith(".test"):
        return stem[:-len(".test")], True
    return stem, False


def find_primary(stem, paths):
    return next((f"{stem}{extension}" for extension in (".hpp", ".h", ".cpp")
                 if f"{stem}{extension}" in paths), None)


def build_sources(files):
    """Builds the sources of a {path: text} mapping and links their modules."""
    paths = set(files)
    sources = {}

    for path, text in files.items():
        stem, is_test = split_module(path)
        primary = find_primary(stem, paths)
        if not is_test and (primary == path or not path.endswith(".cpp")):
            primary = None
        sources[path] = Source(path, text, parse_header(text), is_test, primary)

    return sources


def effective_status(source, sources):
    """Returns the status name governing a source, or None if untagged."""
    owner = sources[source.primary] if source.is_test and source.primary else source
    return owner.header.status_name() if owner.header else None


def review_level(source):
    return source.header.value("reviewed") if source.header else None


def meets_review(level, minimum):
    return level in REVIEW_RANK and REVIEW_RANK[level] >= REVIEW_RANK[minimum]


def check_pair(source, sources):
    """Checks a `.cpp` against the authoritative header of its module."""
    if source.is_test or source.primary is None:
        return []

    primary = sources[source.primary]
    if source.header is None or primary.header is None:
        return []

    findings = []
    if source.header.part_of != primary.header.part_of:
        findings.append(Finding(source.path, source.header.part_of_line, "error",
                                f"'Part of' differs from {primary.path}"))

    own_brief, primary_brief = source.header.brief, primary.header.brief
    if own_brief and primary_brief and own_brief.value != primary_brief.value:
        findings.append(Finding(source.path, own_brief.line, "error",
                                f"@brief differs from {primary.path}"))

    for name in ("created", "status", "reviewed"):
        own, theirs = source.header.tag(name), primary.header.tag(name)
        if own and theirs and own.value != theirs.value:
            findings.append(Finding(source.path, own.line, "error",
                                    f"@{name} differs from {primary.path}: "
                                    f"'{own.value}' vs '{theirs.value}'"))

    return findings


def requires_test(path, config):
    return not any(fnmatch.fnmatch(path, pattern) for pattern in config.no_test_required)


def find_test(primary_path, sources):
    return next((source for source in sources.values()
                 if source.is_test and source.primary == primary_path), None)


def check_requirements(source, sources, config):
    """Checks what the status of a non-test file demands of its module."""
    status = effective_status(source, sources)
    if source.is_test or status not in MIN_REVIEW:
        return []

    path = source.path
    line = source.header.tag("status").line
    findings = [Finding(path, number, "error", f"{match.group(1)} in {status} code")
                for number, text in enumerate(source.text.splitlines(), start=1)
                if (match := OPEN_WORK.search(text))]

    if source.primary is not None:
        return findings

    minimum = MIN_REVIEW[status]
    if not meets_review(review_level(source), minimum):
        findings.append(Finding(path, line, "error", f"{status} needs @reviewed {minimum} or better"))

    if requires_test(path, config):
        test = find_test(path, sources)
        if test is None:
            findings.append(Finding(path, line, "error", f"{status} needs a sibling *.test.cpp"))
        elif not meets_review(review_level(test), MIN_TEST_REVIEW):
            findings.append(Finding(path, line, "error",
                                    f"{status} needs {test.path} @reviewed "
                                    f"{MIN_TEST_REVIEW} or better"))

    return findings


def resolve_include(quote, target, including_path, sources, config):
    bases = [os.path.dirname(including_path)] if quote == '"' else []
    bases += list(config.include_roots)

    for base in bases:
        candidate = os.path.normpath(os.path.join(base, target))
        if candidate in sources:
            return candidate

    return None


def check_includes(source, sources, config):
    """Warns about mature code leaning on less mature or deprecated code."""
    status = effective_status(source, sources)
    if source.is_test or status is None:
        return []

    findings = []
    for number, text in enumerate(source.text.splitlines(), start=1):
        match = INCLUDE_LINE.match(text)
        target = match and resolve_include(match.group(1), match.group(2), source.path,
                                           sources, config)
        if not target or target == source.primary:
            continue

        target_status = effective_status(sources[target], sources)
        if target_status == "deprecated" and status != "deprecated":
            findings.append(Finding(source.path, number, "warning",
                                    f"includes deprecated {target}"))
        elif status in STATUS_RANK and STATUS_RANK[status] >= STABLE_RANK:
            target_rank = STATUS_RANK.get(target_status, 0)
            if target_rank < STATUS_RANK[status]:
                shown = target_status or "untagged"
                findings.append(Finding(source.path, number, "warning",
                                        f"{status} code includes {shown} {target}"))

    return findings


class GitHistory:
    """Answers questions about what happened to a file since a tag last changed."""

    def __init__(self, root):
        self.root = root

    def _git(self, *args):
        result = subprocess.run(["git", "-C", str(self.root), *args],
                                capture_output=True, check=False)
        if result.returncode != 0:
            return ""
        return result.stdout.decode("utf-8", "surrogateescape")

    def first_commit_date(self, path):
        dates = self._git("log", "--follow", "--diff-filter=A", "--format=%as", "--", path)
        return dates.split()[-1] if dates.split() else None

    def last_commit_date(self, path):
        return self._git("log", "-1", "--format=%as", "--", path).strip() or None

    def head_text(self, path):
        """Returns the committed content of a file, or None if there isn't any."""
        return self._git("show", f"HEAD:{path}") or None

    def commits_since_tag(self, path, tag):
        """Returns (subject, changed lines) per commit after `tag` last changed.

        Returns None if the tag isn't committed yet.
        """
        anchor = self._git("log", "-1", "--follow", "--format=%H",
                           f"-G^ \\* @{tag}\\b", "--", path).strip()
        if not anchor:
            return None

        log = self._git("log", "--follow", "--numstat", "--format=%x00%s",
                        f"{anchor}..HEAD", "--", path)

        commits = []
        for entry in log.split("\0")[1:]:
            subject, _, stats = entry.partition("\n")
            changed = sum(int(count)
                          for row in stats.splitlines() if row.strip()
                          for count in row.split("\t")[:2] if count.isdigit())
            commits.append((subject, changed))

        return commits


def check_history(source, sources, config, history):
    """Warns when the code has moved on from what its tags claim."""
    findings = []
    header = source.header
    if header is None:
        return findings

    reviewed = header.tag("reviewed")
    if reviewed and reviewed.value in REVIEW_RANK and reviewed.value != "none":
        commits = history.commits_since_tag(source.path, "reviewed")
        churn = sum(changed for _, changed in commits or [])
        if churn >= config.stale_lines:
            findings.append(Finding(source.path, reviewed.line, "warning",
                                    f"stale review: {churn} lines changed since "
                                    f"@reviewed {reviewed.value} was set"))

    status = header.tag("status")
    name = header.status_name()
    if status is None or name not in MIN_REVIEW:
        return findings

    commits = history.commits_since_tag(source.path, "status") or []
    churn = sum(changed for _, changed in commits)
    if churn >= config.stale_lines:
        findings.append(Finding(source.path, status.line, "warning",
                                f"stale status: {churn} lines changed since @status {name} was set"))

    if name == "frozen":
        fix_commit = re.compile(config.fix_commit_pattern)
        others = [subject for subject, _ in commits if not fix_commit.search(subject)]
        if others:
            findings.append(Finding(source.path, status.line, "warning",
                                    f"frozen, but {len(others)} non-fix commit(s) since: "
                                    f"'{others[0]}'"))

    return findings


def check_all(sources, config, root, history, selection=()):
    """Checks the selected sources; the others only serve as their context."""
    findings = []
    for source in sources.values():
        if not is_selected(source.path, selection):
            continue

        findings += check_format(source, sources, config, root)
        findings += check_pair(source, sources)
        findings += check_requirements(source, sources, config)
        findings += check_includes(source, sources, config)
        findings += check_history(source, sources, config, history)

    return sorted(findings, key=lambda finding: (finding.path, finding.line))


@dataclass
class HeaderParts:
    part_of: str | None = None
    brief: list[str] = field(default_factory=list)
    details: list[list[str]] = field(default_factory=list)
    fields: dict = field(default_factory=dict)
    trailing: list[list[str]] = field(default_factory=list)


@dataclass(frozen=True)
class HeaderFacts:
    """What a fixed header should say where the file doesn't already say it."""
    project: str | None
    brief: str | None
    author: str
    license: str
    created: str | None
    updated: str | None
    status: str | None
    reviewed: str


def split_comment(text):
    """Splits a file into its leading block comment and the rest."""
    lines = text.splitlines(keepends=True)
    if lines and lines[0].rstrip() == "/**":
        for index, line in enumerate(lines):
            if line.strip() == "*/":
                return "".join(lines[:index + 1]), "".join(lines[index + 1:])

    return "", text


def split_paragraphs(comment):
    paragraphs = [[]]
    for line in comment.splitlines()[1:-1]:
        content = comment_content(line)
        if content:
            paragraphs[-1].append(content)
        elif paragraphs[-1]:
            paragraphs.append([])

    return [paragraph for paragraph in paragraphs if paragraph]


def field_of(line):
    """Returns (name, value) if the line is a field of the tag block."""
    license_match = LICENSE_LINE.match(line)
    if license_match:
        return "license", license_match.group(1).strip()

    tag_match = TAG_LINE.match(line)
    if tag_match and tag_match.group(1) == "date":
        return "created", tag_match.group(2).strip()
    if tag_match and tag_match.group(1) in FIELD_ORDER:
        return tag_match.group(1), tag_match.group(2).strip()

    return None


def split_header(comment):
    """Sorts the lines of a header into its parts, losing none of them."""
    parts = HeaderParts()
    paragraphs = split_paragraphs(comment)

    if paragraphs and (match := PART_OF_LINE.match(paragraphs[0][0])):
        parts.part_of = match.group(1).rstrip(" .")
        paragraphs[0].pop(0)

    seen_fields = False
    for paragraph in paragraphs:
        before, after = [], []
        in_brief = False
        for line in paragraph:
            tag_field = field_of(line)
            tag_match = TAG_LINE.match(line)
            duplicate = (tag_field and tag_field[0] in parts.fields
                         and tag_field[0] not in REPEATABLE)
            if tag_field and not duplicate:
                parts.fields.setdefault(tag_field[0], []).append(tag_field[1])
                seen_fields = True
                in_brief = False
            elif tag_match and tag_match.group(1) == "brief" and not parts.brief:
                parts.brief = [tag_match.group(2).strip()]
                in_brief = True
            elif in_brief and not tag_match:
                parts.brief.append(line.strip())
            else:
                in_brief = False
                (after if seen_fields else before).append(line)

        if before:
            parts.details.append(before)
        if after:
            parts.trailing.append(after)

    parts.brief = [line for line in parts.brief if line]
    if not parts.brief and parts.details and not parts.details[0][0].startswith("@"):
        parts.brief = [line.strip() for line in parts.details.pop(0)]

    return parts


def complete_parts(parts, facts):
    """Fills in what is missing. Values that are already there are kept."""
    fields = parts.fields

    parts.part_of = facts.project or parts.part_of
    if not parts.brief and facts.brief:
        parts.brief = [facts.brief]

    authors = fields.get("author", [""])[0].split(" and ")
    if authors[0]:
        fields["author"] = authors[:1]
        fields["coauthor"] = [*authors[1:], *fields.get("coauthor", [])]
    elif facts.author:
        fields["author"] = [facts.author]

    if facts.updated:
        fields["updated"] = [facts.updated]
    if facts.created:
        fields.setdefault("created", [facts.created])
    fields.setdefault("reviewed", [facts.reviewed])
    fields.setdefault("license", [facts.license])

    if facts.status is None:
        fields.pop("status", None)
    else:
        fields.setdefault("status", [facts.status])

    return parts


def render_field(name, value):
    if name == "license":
        return f"{LICENSE_TAG} {value}"
    return f"{'@' + name:<{TAG_WIDTH}}{value}".rstrip()


def render_header(parts, newline):
    brief = [f"{'@brief':<{BRIEF_WIDTH}}{line}".rstrip() if index == 0
             else " " * BRIEF_WIDTH + line
             for index, line in enumerate(parts.brief or [""])]
    tags = [render_field(name, value)
            for name in FIELD_ORDER for value in parts.fields.get(name, [])]

    paragraphs = [[f"Part of {parts.part_of}"]] if parts.part_of else []
    paragraphs += [brief, *parts.details, tags, *parts.trailing]

    lines = ["/**"]
    for index, paragraph in enumerate(paragraphs):
        if index:
            lines.append(" *")
        lines += [f" * {line}" for line in paragraph]

    return newline.join([*lines, " */", ""])


def fix_text(text, facts):
    """Returns the file with a complete, uniform header and an untouched rest."""
    comment, rest = split_comment(text)
    newline = "\r\n" if "\r\n" in text else "\n"
    header = render_header(complete_parts(split_header(comment), facts), newline)

    if comment:
        return header + rest
    return header + (newline + rest if rest else "")


def is_iso_date(value):
    try:
        return bool(value and ISO_DATE.match(value) and date.fromisoformat(value))
    except ValueError:
        return False


def created_of(source, history, today):
    fields = split_header(split_comment(source.text)[0]).fields
    return fields.get("created", [None])[0] or history.first_commit_date(source.path) or today


def code_of(text):
    """Returns what follows the header, without the blank lines separating them."""
    return split_comment(text)[1].lstrip("\r\n")


def updated_of(source, created, history, today):
    """Returns the date of the last change of the code, not of the header."""
    existing = source.header.value("updated") if source.header else None
    committed = history.head_text(source.path)

    if committed is not None and code_of(committed) != code_of(source.text):
        updated = today
    elif is_iso_date(existing):
        updated = existing
    else:
        updated = history.last_commit_date(source.path) or today

    return max(updated, created) if is_iso_date(created) else updated


def brief_for(source, sources):
    """Returns a @brief for a file that has no description of its own."""
    if source.primary is None:
        return None

    primary = sources[source.primary]
    if source.is_test:
        return f"Tests of `{os.path.basename(primary.path)}`."

    brief = split_header(split_comment(primary.text)[0]).brief
    return " ".join(brief) or None


def facts_for(source, sources, config, history, today):
    primary = sources[source.primary] if source.primary else None
    partner = primary if primary and not source.is_test else None

    created = created_of(partner or source, history, today)
    if source.header and source.header.tag("created"):
        created = source.header.value("created")

    status = DEFAULT_STATUS
    reviewed = DEFAULT_REVIEW
    if partner and partner.header:
        status = partner.header.value("status") or status
        reviewed = partner.header.value("reviewed") or reviewed

    return HeaderFacts(project=expected_project(source.path, config),
                       brief=brief_for(source, sources),
                       author=config.author,
                       license=config.license,
                       created=created,
                       updated=updated_of(source, created, history, today),
                       status=status if carries_status(source.path, config)
                       and not (source.is_test and primary) else None,
                       reviewed=reviewed)


def fix_sources(sources, selection, config, history, today):
    """Returns ({path: fixed text} of the changed files, the paths it had to skip)."""
    fixed = {}
    skipped = []

    for path, source in sources.items():
        if not is_selected(path, selection):
            continue

        facts = facts_for(source, sources, config, history, today)
        if source.header is None and facts.project is None:
            skipped.append(path)
            continue

        text = fix_text(source.text, facts)
        if text != source.text:
            fixed[path] = text

    return fixed, skipped


def read_source(path):
    """Reads a file so that writing it back reproduces every byte."""
    return path.read_bytes().decode("utf-8", "surrogateescape")


def write_source(path, text):
    path.write_bytes(text.encode("utf-8", "surrogateescape"))


def run_fix(root, files, selection, config, dry_run):
    history = GitHistory(root)
    fixed, skipped = fix_sources(build_sources(files), selection, config, history,
                                 date.today().isoformat())

    if not dry_run:
        for path, text in fixed.items():
            write_source(root / path, text)

    for path in skipped:
        print(f"{path}:1: skipped: no header, and no project configured for this path")

    verb = "Would fix" if dry_run else "Fixed"
    print(f"{verb} {len(fixed)} file(s), skipped {len(skipped)}. What remains:")

    sources = build_sources(files | fixed)
    findings = check_all(sources, config, root, history, selection)
    print_check(findings)
    return 1 if any(finding.severity == "error" for finding in findings) else 0


@dataclass(frozen=True)
class StagedFile:
    path: str
    previous_path: str | None
    mode: str


def parse_staged(raw_diff):
    """Parses `git diff --raw -z` into staged files; `previous_path` is None if new."""
    tokens = raw_diff.split("\0")
    staged = []
    index = 0
    while index < len(tokens) and tokens[index]:
        _, mode, _, _, status = tokens[index].split()
        if status[0] in "RC":
            staged.append(StagedFile(tokens[index + 2], tokens[index + 1], mode))
            index += 3
        else:
            previous_path = None if status == "A" else tokens[index + 1]
            staged.append(StagedFile(tokens[index + 1], previous_path, mode))
            index += 2

    return staged


def with_updated(text, today):
    """Returns the file with its @updated set, changing nothing else."""
    comment, rest = split_comment(text)
    bumped = re.sub(r"^( \* @updated\s+)\S*", rf"\g<1>{today}", comment, count=1,
                    flags=re.MULTILINE)
    return bumped + rest


def needs_bump(staged_text, committed_text):
    return committed_text is not None and code_of(staged_text) != code_of(committed_text)


class GitIndex:
    """The staged content of a repository."""

    def __init__(self, root):
        self.root = root

    def _git(self, *args, stdin=None):
        result = subprocess.run(["git", "-C", str(self.root), *args], input=stdin,
                                capture_output=True, check=False)
        if result.returncode != 0:
            raise RuntimeError(f"git {args[0]}: {result.stderr.decode(errors='replace').strip()}")
        return result.stdout.decode("utf-8", "surrogateescape")

    def is_replaying(self):
        """Tells if this commit concludes a merge, a cherry-pick, or the like."""
        return any(subprocess.run(["git", "-C", str(self.root), "rev-parse", "-q", "--verify",
                                   reference], capture_output=True, check=False).returncode == 0
                   for reference in ("MERGE_HEAD", "CHERRY_PICK_HEAD", "REVERT_HEAD",
                                     "REBASE_HEAD"))

    def staged_files(self):
        return parse_staged(self._git("diff", "--cached", "--raw", "-z", "-M", "--no-abbrev",
                                      "--diff-filter=ACMR"))

    def read(self, path):
        return self._git("show", f":{path}")

    def write(self, staged, text):
        data = text.encode("utf-8", "surrogateescape")
        blob = self._git("hash-object", "-w", "--stdin", stdin=data).strip()
        self._git("update-index", "--cacheinfo", f"{staged.mode},{blob},{staged.path}")


def bump_staged(staged, staged_text, index, history, root, today):
    """Moves @updated in the index and the working tree if the code changed."""
    committed = history.head_text(staged.previous_path) if staged.previous_path else None
    if not needs_bump(staged_text, committed):
        return staged_text

    bumped = with_updated(staged_text, today)
    if bumped == staged_text:
        return staged_text

    index.write(staged, bumped)

    working_path = Path(root) / staged.path
    if working_path.is_file():
        working_text = read_source(working_path)
        working_bumped = with_updated(working_text, today)
        if working_bumped != working_text:
            write_source(working_path, working_bumped)

    return bumped


def run_pre_commit(root, config):
    index = GitIndex(root)
    staged_files = [staged for staged in index.staged_files() if is_source(staged.path, config)]
    if not staged_files:
        return 0

    history = GitHistory(root)
    today = date.today().isoformat()
    keep_dates = os.environ.get(NO_BUMP_VARIABLE) == "1" or index.is_replaying()

    staged_texts = {}
    for staged in staged_files:
        text = index.read(staged.path)
        if not keep_dates:
            text = bump_staged(staged, text, index, history, root, today)
        staged_texts[staged.path] = text

    files = {path: read_source(root / path) for path in list_files(root, config)} | staged_texts
    findings = check_all(build_sources(files), config, root, history, list(staged_texts))
    if not findings:
        return 0

    print_check(findings)
    if any(finding.severity == "error" for finding in findings):
        print("Commit stopped by the file headers. Try `file_headers.py --fix`, "
              "or skip with `git commit --no-verify`.", file=sys.stderr)
        return 1

    return 0


def render_hook(script):
    quoted = shlex.quote(str(script))
    return f"""#!/bin/sh
{HOOK_MARKER}
if [ ! -x {quoted} ]; then
    echo "file headers: {script} is missing, not checking" >&2
    exit 0
fi
exec {quoted} --pre-commit
"""


def install_hook(root):
    hooks = root / GitIndex(root)._git("rev-parse", "--git-path", "hooks").strip()
    hook = hooks / "pre-commit"

    if hook.exists() and HOOK_MARKER not in hook.read_text(errors="replace"):
        print(f"error: {hook} exists and isn't ours: add this to it instead:\n"
              f"{shlex.quote(str(Path(__file__).resolve()))} --pre-commit", file=sys.stderr)
        return 2

    hooks.mkdir(parents=True, exist_ok=True)
    hook.write_text(render_hook(Path(__file__).resolve()))
    hook.chmod(0o755)
    print(f"Installed {hook}")
    return 0


def is_selected(path, selection):
    return not selection or any(path == item or path.startswith(item.rstrip("/") + "/")
                                for item in selection)


def print_check(findings):
    for finding in findings:
        print(f"{finding.path}:{finding.line}: {finding.severity}: {finding.message}")

    severities = Counter(finding.severity for finding in findings)
    files = len({finding.path for finding in findings})
    print(f"{severities['error']} error(s), {severities['warning']} warning(s) in {files} file(s)")


def group_of(path):
    return path.split("/")[0] if "/" in path else "."


def print_table(title, columns, rows):
    """Prints {group: Counter} rows with a total row. Zeros are shown as dots."""
    label_width = max([len("Total"), *map(len, rows)])
    print(title)
    print(f"  {'':<{label_width}}" + "".join(f"{column:>14}" for column in columns))

    total = Counter()
    for label in sorted(rows):
        total.update(rows[label])
        cells = "".join(f"{rows[label][column] or '.':>14}" for column in columns)
        print(f"  {label:<{label_width}}{cells}")

    cells = "".join(f"{total[column] or '.':>14}" for column in columns)
    print(f"  {'Total':<{label_width}}{cells}\n")


def print_report(sources, findings, config):
    statuses = {}
    reviews = {}
    for source in sources.values():
        group = group_of(source.path)
        status = effective_status(source, sources)
        review = review_level(source)
        if not carries_status(source.path, config):
            status = "exempt"
        elif status not in STATUSES:
            status = "untagged"
        statuses.setdefault(group, Counter())[status] += 1
        reviews.setdefault(group, Counter())[review if review in REVIEW_RANK else "untagged"] += 1

    print_table("Files by @status (tests count as their module)",
                [*STATUSES, "untagged", "exempt"], statuses)
    print_table("Files by @reviewed", [*REVIEW_RANK, "untagged"], reviews)

    failing = Counter(group_of(finding.path) for finding in
                      {(finding.path, finding.severity): finding for finding in findings
                       if finding.severity == "error"}.values())
    print("Files with header errors")
    for group in sorted(statuses):
        print(f"  {group}: {failing[group]} of {sum(statuses[group].values())}")

    warnings = [finding for finding in findings if finding.severity == "warning"]
    print(f"\nAttention ({len(warnings)})")
    for finding in warnings:
        print(f"  {finding.path}:{finding.line}: {finding.message}")


def is_source(path, config):
    return (path.endswith(tuple(config.extensions))
            and not any(fnmatch.fnmatch(path, pattern) for pattern in config.exclude))


def list_files(root, config):
    """Returns the tracked and new source files, without ignored ones."""
    output = subprocess.run(["git", "-C", str(root), "ls-files", "--cached", "--others",
                             "--exclude-standard", "-z"],
                            capture_output=True, text=True, check=True).stdout

    paths = [path for path in output.split("\0") if is_source(path, config)]
    return sorted(path for path in paths if (Path(root) / path).is_file())


def repository_root(directory):
    result = subprocess.run(["git", "-C", str(directory), "rev-parse", "--show-toplevel"],
                            capture_output=True, text=True, check=False)
    return Path(result.stdout.strip()).resolve() if result.returncode == 0 else None


def resolve_targets(paths):
    """Returns the repository the paths are in, and the paths relative to it.

    The paths are relative to the working directory. Without any, the target is
    the whole repository of the working directory. Raises ValueError if the
    paths don't exist, aren't in a repository, or span several repositories.
    """
    if not paths:
        root = repository_root(Path.cwd())
        if root is None:
            raise ValueError("not inside a git repository")
        return root, []

    roots = set()
    targets = []
    for path in paths:
        target = Path(path).resolve()
        if not target.exists():
            raise ValueError(f"no such file or directory: {path}")

        root = repository_root(target if target.is_dir() else target.parent)
        if root is None:
            raise ValueError(f"not inside a git repository: {path}")

        roots.add(root)
        targets.append(target)

    if len(roots) > 1:
        raise ValueError("the paths span several repositories")

    root = roots.pop()
    selection = [target.relative_to(root).as_posix() for target in targets]
    return root, [] if "." in selection else selection


def parse_arguments():
    parser = argparse.ArgumentParser(description="Check C++ file headers and report maturity.")
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--check", action="store_true",
                      help="list problems and fail on errors (default)")
    mode.add_argument("--report", action="store_true",
                      help="summarise maturity per library; never fails on findings")
    mode.add_argument("--fix", action="store_true",
                      help="rewrite headers, then list what still needs a human")
    mode.add_argument("--pre-commit", action="store_true",
                      help="run as the pre-commit hook: move @updated, check the staged files")
    mode.add_argument("--install-hook", action="store_true",
                      help="install the pre-commit hook in the repository")
    parser.add_argument("--dry-run", action="store_true",
                        help="with --fix, write nothing and show what would remain")
    parser.add_argument("paths", nargs="*",
                        help="files or directories to check, in any one repository "
                             "(default: the repository of the working directory)")
    arguments = parser.parse_args()
    if arguments.dry_run and not arguments.fix:
        parser.error("--dry-run needs --fix")
    if arguments.pre_commit and arguments.paths:
        parser.error("--pre-commit works on the staged files of the working directory")
    return arguments


def main():
    arguments = parse_arguments()

    try:
        root, selection = resolve_targets(arguments.paths)
    except ValueError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2

    try:
        config = load_config(root / CONFIG_NAME)
    except (OSError, ValueError, TypeError, tomllib.TOMLDecodeError) as error:
        print(f"error: cannot load {root / CONFIG_NAME}: {error}", file=sys.stderr)
        return 2

    if arguments.install_hook:
        return install_hook(root)
    if arguments.pre_commit:
        return run_pre_commit(root, config)

    files = {path: read_source(root / path) for path in list_files(root, config)}
    sources = build_sources(files)
    selected_sources = {path: source for path, source in sources.items()
                        if is_selected(path, selection)}
    if not selected_sources:
        print(f"error: no source files to check in {root}", file=sys.stderr)
        return 2

    if arguments.fix:
        return run_fix(root, files, selection, config, arguments.dry_run)

    findings = check_all(sources, config, root, GitHistory(root), selection)
    if arguments.report:
        print_report(selected_sources, findings, config)
        return 0

    print_check(findings)
    return 1 if any(finding.severity == "error" for finding in findings) else 0


if __name__ == "__main__":
    sys.exit(main())
