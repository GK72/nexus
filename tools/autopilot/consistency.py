#!/usr/bin/env python3
#
# Part of Nexus Library
#
# Structure check of a C++ repository against the project template
# (templates/cpp-project): what is missing, what is legacy, what drifted, and
# whether it builds on the current Nova. Deterministic and token-free: autopilot
# hands its findings to the surveyor agent instead of letting it explore.
#
#   consistency.py ~/repos/fdm            findings, one per line
#   consistency.py ~/repos/fdm --json     the same as JSON
#
# Exit status: 0 no errors, 1 errors found, 2 usage.

import argparse
import json
import re
import sys
from dataclasses import dataclass, asdict
from pathlib import Path
from typing import Callable, Iterable

TEMPLATE = Path(__file__).resolve().parents[2] / 'templates' / 'cpp-project'

# Files every project has, as the template has them.
REQUIRED = [
    'README.md', 'version.txt', 'conanfile.py', 'conandata.yml', 'dependencies.cmake',
    'settings.cmake', 'ci/pipeline.sh', 'tests/common.sh', 'cmake/test.cmake',
    'tools/configure.sh', 'tools/doc.sh', '.file-headers.toml', '.shellcheckrc', '.gitignore',
]

# Legacy file -> what replaced it.
LEGACY = {
    'VERSION': 'version.txt',
    'conanfile.txt': 'conanfile.py + conandata.yml',
    'CMakeSettings.json': 'CMakeUserPresets.json (generated)',
    'conan_provider.cmake': 'tools/configure.sh',
}

# Files that carry no placeholders and should equal the template's copy.
SHARED = ['.shellcheckrc', 'cmake/test.cmake', 'tools/configure.sh', 'tools/doc.sh', 'tests/common.sh']

NOVA_REF = re.compile(r'nova/(\d+\.\d+\.\d+)(@[\w./-]+)?')


@dataclass
class Finding:
    severity: str   # error | warning | info
    check: str
    message: str


def nova_versions(text: str) -> list[str]:
    return [m.group(1) + (m.group(2) or '') for m in NOVA_REF.finditer(text)]


def version_key(version: str) -> tuple[int, ...]:
    return tuple(int(part) for part in version.split('@')[0].split('.'))


def check(files: set[str], read: Callable[[str], str], template_read: Callable[[str], str],
          template_nova: str) -> list[Finding]:
    """Pure core: `files` are the repository's relative paths, `read` returns one's text."""
    findings: list[Finding] = []
    if 'CMakeLists.txt' not in files:
        return [Finding('info', 'not-cpp', 'no CMakeLists.txt: not a C++ project, nothing checked')]

    for path in REQUIRED:
        if path not in files:
            findings.append(Finding('error', 'missing', f'{path} (from the template)'))
    for path, successor in LEGACY.items():
        if path in files:
            findings.append(Finding('error', 'legacy', f'{path}: replaced by {successor}'))

    old_ci = sorted(p for p in files if p.startswith('ci/') and p.endswith('.sh') and p != 'ci/pipeline.sh')
    if old_ci:
        findings.append(Finding('error', 'ci', f'{len(old_ci)} pre-nci scripts ({", ".join(old_ci[:4])}'
                                f'{", ..." if len(old_ci) > 4 else ""}): move into ci/pipeline.sh stages'))
    if 'ci/pipeline.sh' in files and 'nci.sh' not in read('ci/pipeline.sh'):
        findings.append(Finding('error', 'ci', 'ci/pipeline.sh does not use nci (tools/ci/nci.sh)'))

    for path in SHARED:
        if path in files and read(path) != template_read(path):
            findings.append(Finding('warning', 'drift', f'{path} differs from the template'))

    refs = [v for p in ('conanfile.py', 'conanfile.txt', 'conandata.yml') if p in files for v in nova_versions(read(p))]
    if not refs:
        findings.append(Finding('error', 'nova', 'does not depend on Nova'))
    for ref in sorted(set(refs)):
        if '@' in ref:
            findings.append(Finding('info', 'nova', f'builds on a tagged Nova: nova/{ref}'))
        elif version_key(ref) < version_key(template_nova):
            findings.append(Finding('warning', 'nova', f'nova/{ref}, the template uses {template_nova}'))
    return findings


def repository_files(root: Path) -> set[str]:
    skip = {'.git', 'build', 'deps', '.cache', '.tmp', '.claude', 'node_modules'}
    files = set()
    for path in root.rglob('*'):
        relative = path.relative_to(root)
        if relative.parts[0] in skip or not path.is_file():
            continue
        files.add(relative.as_posix())
    return files


def template_nova_version() -> str:
    versions = nova_versions((TEMPLATE / 'conandata.yml').read_text()) or nova_versions((TEMPLATE / 'conanfile.py').read_text())
    return max(versions, key=version_key) if versions else '0.0.0'


def run(root: Path) -> list[Finding]:
    def reader(base: Path) -> Callable[[str], str]:
        return lambda path: (base / path).read_text(errors='replace') if (base / path).is_file() else ''
    return check(repository_files(root), reader(root), reader(TEMPLATE), template_nova_version())


def format_lines(findings: Iterable[Finding]) -> str:
    return '\n'.join(f'{f.severity:<7} {f.check:<8} {f.message}' for f in findings)


def main() -> int:
    parser = argparse.ArgumentParser(description='Check a repository against the C++ project template.')
    parser.add_argument('repo', type=Path)
    parser.add_argument('--json', action='store_true')
    args = parser.parse_args()
    root = args.repo.expanduser().resolve()
    if not root.is_dir():
        print(f'consistency: {root} is not a directory', file=sys.stderr)
        return 2
    findings = run(root)
    print(json.dumps([asdict(f) for f in findings], indent=1) if args.json else format_lines(findings) or 'consistent')
    return 1 if any(f.severity == 'error' for f in findings) else 0


if __name__ == '__main__':
    sys.exit(main())
