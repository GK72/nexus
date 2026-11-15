#!/usr/bin/env python3
#
# Part of Nexus Library
#
# Unit tests of the consistency checker's pure core: repositories are in-memory dicts.

import unittest

from consistency import REQUIRED, check

TEMPLATE = {path: f'template {path}' for path in REQUIRED}
TEMPLATE['ci/pipeline.sh'] = 'source tools/ci/nci.sh'
TEMPLATE['conandata.yml'] = 'requirements:\n  - nova/0.15.0\n'


def run(repo: dict[str, str]):
    return check(set(repo), lambda p: repo.get(p, ''), lambda p: TEMPLATE.get(p, ''), '0.15.0')


def checks(findings):
    return sorted((f.severity, f.check) for f in findings)


class ConsistencyTest(unittest.TestCase):
    def test_template_copy_is_consistent(self):
        self.assertEqual(run({'CMakeLists.txt': '', **TEMPLATE}), [])

    def test_not_cpp(self):
        self.assertEqual(checks(run({'README.md': ''})), [('info', 'not-cpp')])

    def test_legacy_and_missing(self):
        repo = {'CMakeLists.txt': '', **TEMPLATE, 'VERSION': '1'}
        del repo['version.txt']
        self.assertEqual(checks(run(repo)), [('error', 'legacy'), ('error', 'missing')])

    def test_old_ci_scripts(self):
        findings = run({'CMakeLists.txt': '', **TEMPLATE, 'ci/build.sh': ''})
        self.assertEqual(checks(findings), [('error', 'ci')])

    def test_pipeline_without_nci(self):
        findings = run({'CMakeLists.txt': '', **TEMPLATE, 'ci/pipeline.sh': 'make'})
        self.assertEqual(checks(findings), [('error', 'ci')])

    def test_drift(self):
        findings = run({'CMakeLists.txt': '', **TEMPLATE, 'tests/common.sh': 'changed'})
        self.assertEqual(checks(findings), [('warning', 'drift')])

    def test_nova_versions(self):
        old = run({'CMakeLists.txt': '', **TEMPLATE, 'conandata.yml': '- nova/0.12.0'})
        self.assertEqual(checks(old), [('warning', 'nova')])
        tagged = run({'CMakeLists.txt': '', **TEMPLATE, 'conandata.yml': '- nova/0.16.0@chris/autopilot'})
        self.assertEqual(checks(tagged), [('info', 'nova')])
        none = run({'CMakeLists.txt': '', **TEMPLATE, 'conandata.yml': '- gtest/1.14.0'})
        self.assertEqual(checks(none), [('error', 'nova')])


if __name__ == '__main__':
    unittest.main()
