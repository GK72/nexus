#!/usr/bin/env python3
"""Tests of file_headers.py. Run with `python3 tools/file_headers_test.py`."""

import contextlib
import dataclasses
import io
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))

import file_headers as fh

CONFIG = fh.Config(projects={"lib": "Nexus Library", "lib/special": "Special"},
                   no_test_required=("*/main.cpp",), no_status=("*.ex.cpp",), stale_lines=10,
                   author="Someone")


def header(**overrides):
    """Returns a valid header; a value of None drops that line."""
    fields = {
        "part_of": "Part of Nexus Library",
        "brief": "@brief   Does a thing.",
        "author": "@author      Someone",
        "created": "@created     2026-01-01",
        "updated": "@updated     2026-02-01",
        "status": "@status      evolving",
        "reviewed": "@reviewed    none",
        "license": "SPDX-License-Identifier: Unlicense",
    } | overrides

    def line(name):
        return [f" * {fields[name]}"] if fields[name] is not None else []

    lines = ["/**", *line("part_of"), " *", *line("brief"), " *"]
    for name in ("author", "created", "updated", "status", "reviewed", "license"):
        lines += line(name)
    return "\n".join([*lines, " */", ""])


class NoHistory:
    def commits_since_tag(self, path, tag):
        return None


class FakeHistory:
    def __init__(self, commits):
        self.commits = commits

    def commits_since_tag(self, path, tag):
        return self.commits


def run(files, history=None, root="."):
    sources = fh.build_sources(files)
    return fh.check_all(sources, CONFIG, root, history or NoHistory())


def messages(files, severity="error", history=None, root="."):
    return [finding.message for finding in run(files, history, root)
            if finding.severity == severity]


class FormatTest(unittest.TestCase):
    def assert_error(self, text, expected, path="lib/a.hpp"):
        found = messages({path: text})
        self.assertTrue(any(expected in message for message in found), found)

    def test_valid_header_is_clean(self):
        self.assertEqual(run({"lib/a.hpp": header()}), [])

    def test_header_may_have_coauthor_and_free_text(self):
        text = header(author="@author      Someone\n * @coauthor    Claude Opus 5.5")
        text = text.replace(" */", " *\n * Free text, with @note in it.\n */")
        self.assertEqual(run({"lib/a.hpp": text}), [])

    def test_missing_header(self):
        self.assert_error("#pragma once\n", "missing file header")

    def test_part_of_is_required_above_brief(self):
        self.assert_error(header(part_of=None), "missing 'Part of <project>'")

    def test_part_of_takes_no_trailing_period(self):
        self.assert_error(header(part_of="Part of Nexus Library."), "expected 'Part of Nexus")

    def test_part_of_is_one_line(self):
        self.assert_error(header(part_of="Part of Nexus Library\n * more"), "single line")

    def test_longest_project_prefix_wins(self):
        text = header(part_of="Part of Special")
        self.assertEqual(run({"lib/special/a.hpp": text}), [])

    def test_unconfigured_project(self):
        self.assert_error(header(), "no project configured", path="other/a.hpp")

    def test_brief_is_required(self):
        self.assert_error(header(brief=None), "missing @brief")

    def test_untagged_description_is_named(self):
        self.assert_error(header(brief="Does a thing."), "description is untagged")

    def test_empty_brief(self):
        self.assert_error(header(brief="@brief"), "@brief is empty")

    def test_brief_may_span_lines(self):
        text = header(brief="@brief   Does a thing\n *          over two lines.")
        self.assertEqual(run({"lib/a.hpp": text}), [])
        self.assertEqual(fh.parse_header(text).brief.value, "Does a thing over two lines.")

    def test_continuation_must_align_with_brief(self):
        text = header(brief="@brief   Does a thing\n *   over two lines.")
        self.assert_error(text, "continuation must start at column 9")

    def test_details_may_sit_between_brief_and_tags(self):
        text = header(brief="@brief   Does a thing.\n *\n * Some details.\n *\n * More.")
        self.assertEqual(run({"lib/a.hpp": text}), [])

    def test_brief_must_be_aligned(self):
        self.assert_error(header(brief="@brief Does a thing."), "misaligned @brief")

    def test_brief_must_be_separate_from_tags(self):
        text = header().replace("Does a thing.\n *\n", "Does a thing.\n")
        self.assert_error(text, "separated from the tags")

    def test_required_tags(self):
        for name in ("author", "created", "updated", "status", "reviewed", "license"):
            with self.subTest(name):
                self.assert_error(header(**{name: None}), f"missing @{name}")

    def test_legacy_date_is_one_finding(self):
        found = messages({"lib/a.hpp": header(created="@date        2026-01-01")})
        self.assertEqual(found, ["@date is replaced by @created"])

    def test_tags_must_be_aligned(self):
        self.assert_error(header(status="@status evolving"), "misaligned @status")

    def test_tags_must_be_ordered(self):
        text = header(status="@reviewed    none", reviewed="@status      evolving")
        self.assert_error(text, "out of order")

    def test_duplicate_tag(self):
        self.assert_error(header(status="@status      evolving\n * @status      stable"), "duplicate")

    def test_unknown_tag(self):
        self.assert_error(header(license="@foo      bar"), "unknown tag @foo")

    def test_tag_outside_block(self):
        text = header().replace(" */", " *\n * @status      evolving\n */")
        self.assert_error(text, "outside the tag block")

    def test_author_is_one_person(self):
        self.assert_error(header(author="@author      Someone and Junie"), "use @coauthor")

    def test_invalid_dates(self):
        for value in ("2026-13-01", "20260101", "soon"):
            with self.subTest(value):
                self.assert_error(header(created=f"@created     {value}"), "not a YYYY-MM-DD")

    def test_created_after_updated(self):
        self.assert_error(header(created="@created     2026-03-01"), "later than @updated")

    def test_unknown_status_and_review(self):
        self.assert_error(header(status="@status      unstable"), "unknown @status")
        self.assert_error(header(reviewed="@reviewed    glanced"), "unknown @reviewed")

    def test_wrong_license(self):
        self.assert_error(header(license="SPDX-License-Identifier: MIT"), "expected license")

    def test_private_project_has_no_license(self):
        private = dataclasses.replace(CONFIG, license="")
        files = {"lib/a.hpp": header(license=None)}
        self.assertEqual(fh.check_all(fh.build_sources(files), private, ".", NoHistory()), [])
        files = {"lib/a.hpp": header()}
        found = [finding.message
                 for finding in fh.check_all(fh.build_sources(files), private, ".", NoHistory())]
        self.assertTrue(any("no license expected" in message for message in found), found)

    def test_malformed_comment_line(self):
        self.assert_error(header().replace(" * @brief", "* @brief"), "must start with ' * '")


class DeprecatedTest(unittest.TestCase):
    def test_replacement_is_required(self):
        found = messages({"lib/a.hpp": header(status="@status      deprecated")})
        self.assertTrue(any("replacement" in message for message in found), found)

    def test_replacement_must_exist(self):
        with tempfile.TemporaryDirectory() as root:
            status = "@status      deprecated lib/new.hpp"
            files = {"lib/a.hpp": header(status=status)}
            self.assertIn("replacement 'lib/new.hpp' does not exist",
                          messages(files, root=root))

            (Path(root) / "lib").mkdir()
            (Path(root) / "lib/new.hpp").touch()
            self.assertEqual(messages(files, root=root), [])

    def test_only_deprecated_takes_an_argument(self):
        found = messages({"lib/a.hpp": header(status="@status      stable lib/b.hpp")})
        self.assertIn("@status stable takes no argument", found)


class ModuleTest(unittest.TestCase):
    def test_cpp_must_match_header(self):
        files = {"lib/a.hpp": header(), "lib/a.cpp": header(reviewed="@reviewed    read")}
        self.assertTrue(any("@reviewed differs" in message for message in messages(files)))

    def test_brief_must_match(self):
        files = {"lib/a.hpp": header(), "lib/a.cpp": header(brief="@brief   Other.")}
        self.assertTrue(any("@brief differs" in message for message in messages(files)))

    def test_updated_may_differ(self):
        files = {"lib/a.hpp": header(), "lib/a.cpp": header(updated="@updated     2026-03-01")}
        self.assertEqual(run(files), [])

    def test_c_header_is_separate_from_cpp_header(self):
        files = {"lib/a.hpp": header(), "lib/a.h": header(brief="@brief   C bindings.")}
        self.assertEqual(run(files), [])

    def test_cpp_pairs_with_c_header_alone(self):
        files = {"lib/a.h": header(), "lib/a.cpp": header(brief="@brief   Other.")}
        self.assertTrue(any("@brief differs from lib/a.h" in m for m in messages(files)))

    def test_test_inherits_status(self):
        files = {"lib/a.hpp": header(), "lib/a.test.cpp": header(status=None)}
        self.assertEqual(run(files), [])

    def test_test_must_not_repeat_status(self):
        files = {"lib/a.hpp": header(), "lib/a.test.cpp": header()}
        self.assertTrue(any("inherit @status" in message for message in messages(files)))

    def test_standalone_test_needs_status(self):
        self.assertIn("missing @status", messages({"lib/a.test.cpp": header(status=None)}))

    def test_example_is_its_own_module(self):
        files = {"lib/a.hpp": header(),
                 "lib/a.ex.cpp": header(brief="@brief   Example.", status=None)}
        self.assertEqual(run(files), [])

    def test_exempt_path_must_not_carry_status(self):
        self.assertEqual(messages({"lib/a.ex.cpp": header()}),
                         ["no @status expected here: remove it"])

    def test_exempt_path_is_not_counted_as_untagged(self):
        sources = fh.build_sources({"lib/a.ex.cpp": header(status=None),
                                    "lib/b.hpp": header(status=None)})
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            fh.print_report(sources, [], CONFIG)

        table = output.getvalue().splitlines()
        columns = table[1].split()
        cells = dict(zip(columns, table[2].split()[1:]))
        self.assertEqual((cells["untagged"], cells["exempt"]), ("1", "1"))


class RequirementTest(unittest.TestCase):
    STABLE = {"status": "@status      stable", "reviewed": "@reviewed    read"}
    TEST = {"status": None, "reviewed": "@reviewed    read"}

    def test_stable_module_is_clean(self):
        files = {"lib/a.hpp": header(**self.STABLE), "lib/a.test.cpp": header(**self.TEST)}
        self.assertEqual(run(files), [])

    def test_stable_needs_test(self):
        found = messages({"lib/a.hpp": header(**self.STABLE)})
        self.assertEqual(found, ["stable needs a sibling *.test.cpp"])

    def test_exempt_path_needs_no_test(self):
        self.assertEqual(run({"lib/main.cpp": header(**self.STABLE)}), [])

    def test_stable_needs_review(self):
        files = {"lib/a.hpp": header(status="@status      stable", reviewed="@reviewed    skimmed"),
                 "lib/a.test.cpp": header(**self.TEST)}
        self.assertEqual(messages(files), ["stable needs @reviewed read or better"])

    def test_stable_needs_reviewed_test(self):
        files = {"lib/a.hpp": header(**self.STABLE), "lib/a.test.cpp": header(status=None)}
        self.assertEqual(messages(files),
                         ["stable needs lib/a.test.cpp @reviewed read or better"])

    def test_frozen_needs_audit(self):
        files = {"lib/a.hpp": header(status="@status      frozen", reviewed="@reviewed    read"),
                 "lib/a.test.cpp": header(**self.TEST)}
        self.assertEqual(messages(files), ["frozen needs @reviewed audited or better"])

    def test_todo_blocks_stable_in_either_file(self):
        files = {"lib/a.hpp": header(**self.STABLE),
                 "lib/a.cpp": header(**self.STABLE) + "// TODO later\n",
                 "lib/a.test.cpp": header(**self.TEST)}
        self.assertEqual(messages(files), ["TODO in stable code"])

    def test_todo_is_fine_while_evolving(self):
        self.assertEqual(run({"lib/a.hpp": header() + "// FIXME\n"}), [])


class IncludeTest(unittest.TestCase):
    def stable(self, body):
        return header(status="@status      stable", reviewed="@reviewed    read") + body

    def files(self, body, target_status):
        return {"lib/a.hpp": self.stable(body), "lib/a.test.cpp":
                header(status=None, reviewed="@reviewed    read"),
                "lib/b.hpp": header(status=target_status)}

    def test_stable_including_less_mature_code_warns(self):
        files = self.files("#include <lib/b.hpp>\n", "@status      experimental")
        self.assertEqual(messages(files, "warning"),
                         ["stable code includes experimental lib/b.hpp"])

    def test_quoted_include_resolves_next_to_the_file(self):
        files = self.files('#include "b.hpp"\n', "@status      evolving")
        self.assertEqual(messages(files, "warning"),
                         ["stable code includes evolving lib/b.hpp"])

    def test_including_deprecated_code_warns_at_any_level(self):
        with tempfile.TemporaryDirectory() as root:
            (Path(root) / "new.hpp").touch()
            files = {"lib/a.hpp": header() + "#include <lib/b.hpp>\n",
                     "lib/b.hpp": header(status="@status      deprecated new.hpp")}
            self.assertEqual(messages(files, "warning", root=root),
                             ["includes deprecated lib/b.hpp"])

    def test_own_header_is_not_a_finding(self):
        files = {"lib/a.hpp": header(), "lib/a.cpp": header() + "#include <lib/a.hpp>\n"}
        self.assertEqual(run(files), [])

    def test_external_includes_are_ignored(self):
        self.assertEqual(run({"lib/a.hpp": header() + "#include <vector>\n"}), [])


class HistoryTest(unittest.TestCase):
    FROZEN = {"status": "@status      frozen", "reviewed": "@reviewed    audited"}

    def files(self):
        return {"lib/a.hpp": header(**self.FROZEN),
                "lib/a.test.cpp": header(status=None, reviewed="@reviewed    read")}

    def test_quiet_history_is_clean(self):
        self.assertEqual(run(self.files(), FakeHistory([("Fix: x", 3)])), [])

    def test_stale_review_and_status(self):
        found = messages(self.files(), "warning", FakeHistory([("Fix: x", 10)]))
        self.assertIn("stale review: 10 lines changed since @reviewed audited was set", found)
        self.assertIn("stale status: 10 lines changed since @status frozen was set", found)

    def test_frozen_warns_on_non_fix_commits(self):
        found = messages(self.files(), "warning", FakeHistory([("Feat: more", 1)]))
        self.assertEqual(found, ["frozen, but 1 non-fix commit(s) since: 'Feat: more'"])

    def test_no_review_has_no_staleness(self):
        files = {"lib/a.hpp": header()}
        self.assertEqual(messages(files, "warning", FakeHistory([("Feat: x", 500)])), [])

    def test_history_issues_never_fail_the_run(self):
        self.assertEqual(messages(self.files(), "error", FakeHistory([("Feat: x", 500)])), [])


class GitHistoryTest(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.git("init", "-q")
        self.git("config", "user.email", "test@example.invalid")
        self.git("config", "user.name", "Test")

    def git(self, *args):
        subprocess.run(["git", "-C", str(self.root), *args], check=True)

    def commit(self, name, text, subject):
        (self.root / name).write_text(text)
        self.git("add", "-A")
        self.git("commit", "-qm", subject)

    def test_counts_only_what_follows_the_tag_change(self):
        self.commit("a.hpp", header(reviewed="@reviewed    none"), "Feat: add")
        self.commit("a.hpp", header(reviewed="@reviewed    read"), "Misc: review")
        self.commit("a.hpp", header(reviewed="@reviewed    read") + "one\ntwo\n", "Feat: grow")

        commits = fh.GitHistory(self.root).commits_since_tag("a.hpp", "reviewed")
        self.assertEqual(commits, [("Feat: grow", 2)])

    def test_updated_bump_does_not_count_as_a_review(self):
        self.commit("a.hpp", header(), "Feat: add")
        self.commit("a.hpp", header(updated="@updated     2026-03-01"), "Feat: touch")

        commits = fh.GitHistory(self.root).commits_since_tag("a.hpp", "reviewed")
        self.assertEqual(commits, [("Feat: touch", 2)])

    def test_uncommitted_tag(self):
        self.commit("b.hpp", "x\n", "Feat: other")
        (self.root / "a.hpp").write_text(header())
        self.assertIsNone(fh.GitHistory(self.root).commits_since_tag("a.hpp", "reviewed"))

    def test_survives_a_rename(self):
        self.commit("a.hpp", header(), "Feat: add")
        self.git("mv", "a.hpp", "b.hpp")
        self.git("commit", "-qm", "Refact: move")
        self.commit("b.hpp", header() + "one\n", "Feat: grow")

        commits = fh.GitHistory(self.root).commits_since_tag("b.hpp", "reviewed")
        self.assertIn(("Feat: grow", 1), commits)


class DatedHistory(NoHistory):
    """A file added on 2026-01-01 and last committed on 2026-02-01."""

    def __init__(self, committed=None):
        self.committed = committed or {}

    def first_commit_date(self, path):
        return "2026-01-01"

    def last_commit_date(self, path):
        return "2026-02-01"

    def head_text(self, path):
        return self.committed.get(path)


TODAY = "2026-06-01"
LEGACY = """/**
 * Part of Nexus.
 *
 * Does a thing
 * over two lines.
 *
 * @author      Someone and Junie
 * @coauthor    Claude
 * @date        2026-01-01
 *
 * Trailing text:
 *   - indented list
 *
 * @compliance  Not complete.
 */

#include <vector>
"""


class FixTest(unittest.TestCase):
    def fix(self, files, history=None, selection=()):
        sources = fh.build_sources(files)
        fixed, skipped = fh.fix_sources(sources, selection, CONFIG, history or DatedHistory(),
                                        TODAY)
        self.skipped = skipped
        return files | fixed

    def test_valid_header_is_left_alone(self):
        files = {"lib/a.hpp": header()}
        self.assertEqual(self.fix(files), files)

    def test_legacy_header_becomes_valid(self):
        fixed = self.fix({"lib/a.hpp": LEGACY})
        self.assertEqual(run(fixed), [])
        self.assertEqual(fixed["lib/a.hpp"], """/**
 * Part of Nexus Library
 *
 * @brief   Does a thing
 *          over two lines.
 *
 * @author      Someone
 * @coauthor    Junie
 * @coauthor    Claude
 * @created     2026-01-01
 * @updated     2026-02-01
 * @status      experimental
 * @reviewed    none
 * SPDX-License-Identifier: Unlicense
 *
 * Trailing text:
 *   - indented list
 *
 * @compliance  Not complete.
 */

#include <vector>
""")

    def test_fix_is_idempotent(self):
        once = self.fix({"lib/a.hpp": LEGACY, "lib/b.cpp": "int x;\n",
                         "lib/b.test.cpp": "int y;\n"})
        self.assertEqual(self.fix(once), once)

    def test_details_stay_between_brief_and_tags(self):
        text = LEGACY.replace(" * @author", " * Details.\n *\n * @author")
        fixed = self.fix({"lib/a.hpp": text})["lib/a.hpp"]
        self.assertIn("over two lines.\n *\n * Details.\n *\n * @author", fixed)

    def test_existing_values_are_never_raised_or_replaced(self):
        text = header(status="@status      Clean", reviewed="@reviewed    skimmed")
        fixed = self.fix({"lib/a.hpp": text})["lib/a.hpp"]
        self.assertIn("@status      Clean\n", fixed)
        self.assertIn("@reviewed    skimmed\n", fixed)

    def test_duplicate_tag_is_kept_for_a_human(self):
        text = header(status="@status      evolving\n * @status      stable")
        fixed = self.fix({"lib/a.hpp": text})
        self.assertIn("@status      stable", fixed["lib/a.hpp"])
        self.assertTrue(any("@status" in message for message in messages(fixed)))

    def test_missing_header_gets_an_empty_brief(self):
        fixed = self.fix({"lib/a.hpp": "#pragma once\n"})
        self.assertTrue(fixed["lib/a.hpp"].endswith(" */\n\n#pragma once\n"))
        self.assertEqual(messages(fixed), ["@brief is empty"])

    def test_unknown_project_without_header_is_skipped(self):
        files = {"other/a.hpp": "#pragma once\n"}
        self.assertEqual(self.fix(files), files)
        self.assertEqual(self.skipped, ["other/a.hpp"])

    def test_test_inherits_and_gets_a_generated_brief(self):
        fixed = self.fix({"lib/a.hpp": header(), "lib/a.test.cpp": "int y;\n"})
        self.assertIn("Tests of `a.hpp`.", fixed["lib/a.test.cpp"])
        self.assertNotIn("@status", fixed["lib/a.test.cpp"])
        self.assertEqual(run(fixed), [])

    def test_test_drops_a_redundant_status(self):
        fixed = self.fix({"lib/a.hpp": header(), "lib/a.test.cpp": header()})
        self.assertEqual(run(fixed), [])

    def test_exempt_path_gets_no_status(self):
        fixed = self.fix({"lib/a.ex.cpp": "int y;\n", "lib/b.ex.cpp": header()})
        self.assertNotIn("@status", fixed["lib/a.ex.cpp"])
        self.assertNotIn("@status", fixed["lib/b.ex.cpp"])

    def test_standalone_test_keeps_its_own_status(self):
        fixed = self.fix({"lib/a.test.cpp": "int y;\n"})
        self.assertIn("@status      experimental", fixed["lib/a.test.cpp"])

    def test_cpp_follows_its_header(self):
        primary = header(status="@status      stable", reviewed="@reviewed    read",
                         created="@created     2025-05-05")
        fixed = self.fix({"lib/a.hpp": primary, "lib/a.cpp": "int x;\n"})
        text = fixed["lib/a.cpp"]
        self.assertIn("@brief   Does a thing.", text)
        self.assertIn("@created     2025-05-05", text)
        self.assertIn("@status      stable", text)
        self.assertIn("@reviewed    read", text)

    def test_cpp_takes_the_description_a_legacy_header_will_get(self):
        fixed = self.fix({"lib/a.hpp": LEGACY, "lib/a.cpp": "int x;\n"})
        self.assertEqual(messages(fixed), [])

    def test_updated_moves_when_the_code_differs_from_head(self):
        history = DatedHistory({"lib/a.hpp": header() + "old\n"})
        fixed = self.fix({"lib/a.hpp": header() + "new\n"}, history)
        self.assertIn(f"@updated     {TODAY}", fixed["lib/a.hpp"])

    def test_header_only_change_does_not_move_updated(self):
        history = DatedHistory({"lib/a.hpp": LEGACY})
        fixed = self.fix({"lib/a.hpp": LEGACY.replace("Part of Nexus.", "Part of Nexus")}, history)
        self.assertIn("@updated     2026-02-01", fixed["lib/a.hpp"])

    def test_a_new_header_is_not_a_code_change(self):
        history = DatedHistory({"lib/a.hpp": "#pragma once\n"})
        once = self.fix({"lib/a.hpp": "#pragma once\n"}, history)
        self.assertIn("@updated     2026-02-01", once["lib/a.hpp"])
        self.assertEqual(self.fix(once, history), once)

    def test_updated_is_never_before_created(self):
        text = LEGACY.replace("2026-01-01", "2026-03-03")
        self.assertIn("@updated     2026-03-03", self.fix({"lib/a.hpp": text})["lib/a.hpp"])

    def test_invalid_created_is_left_for_a_human(self):
        text = header(created="@created     soon")
        fixed = self.fix({"lib/a.hpp": text})
        self.assertIn("@created     soon", fixed["lib/a.hpp"])
        self.assertEqual(messages(fixed), ["@created is not a YYYY-MM-DD date: 'soon'"])

    def test_selection_limits_what_is_fixed(self):
        files = {"lib/a.hpp": LEGACY, "lib/b.hpp": LEGACY}
        fixed = self.fix(files, selection=["lib/a.hpp"])
        self.assertNotEqual(fixed["lib/a.hpp"], LEGACY)
        self.assertEqual(fixed["lib/b.hpp"], LEGACY)

    def test_crlf_files_stay_crlf(self):
        text = LEGACY.replace("\n", "\r\n")
        fixed = self.fix({"lib/a.hpp": text})["lib/a.hpp"]
        self.assertEqual(fixed.count("\n"), fixed.count("\r\n"))

    def test_rest_of_the_file_is_byte_exact(self):
        with tempfile.TemporaryDirectory() as root:
            path = Path(root) / "a.hpp"
            path.write_bytes(b"int a;\r\n// caf\xe9\n\x0c\n")
            fixed = fh.fix_text(fh.read_source(path), fh.HeaderFacts(
                "Nexus Library", None, "Someone", "Unlicense", "2026-01-01", "2026-01-01",
                "experimental", "none"))
            fh.write_source(path, fixed)
            self.assertTrue(path.read_bytes().endswith(b"int a;\r\n// caf\xe9\n\x0c\n"))


class StagedParsingTest(unittest.TestCase):
    def test_parses_each_kind_of_change(self):
        raw = "\0".join([":000000 100644 0 1 A", "new.hpp",
                         ":100644 100755 1 2 M", "changed.hpp",
                         ":100644 100644 1 2 R095", "old.hpp", "moved.hpp", ""])
        self.assertEqual(fh.parse_staged(raw), [
            fh.StagedFile("new.hpp", None, "100644"),
            fh.StagedFile("changed.hpp", "changed.hpp", "100755"),
            fh.StagedFile("moved.hpp", "old.hpp", "100644")])

    def test_nothing_staged(self):
        self.assertEqual(fh.parse_staged(""), [])


class BumpTest(unittest.TestCase):
    def test_only_the_header_date_changes(self):
        text = header() + "// @updated     2020-01-01\n"
        bumped = fh.with_updated(text, TODAY)
        self.assertIn(f" * @updated     {TODAY}\n", bumped)
        self.assertTrue(bumped.endswith("// @updated     2020-01-01\n"))
        self.assertEqual(len(bumped), len(text))

    def test_keeps_crlf(self):
        text = header().replace("\n", "\r\n")
        self.assertIn(f"@updated     {TODAY}\r\n", fh.with_updated(text, TODAY))

    def test_no_header_means_no_change(self):
        self.assertEqual(fh.with_updated("int x;\n", TODAY), "int x;\n")

    def test_code_change_needs_a_bump(self):
        self.assertTrue(fh.needs_bump(header() + "new\n", header() + "old\n"))

    def test_header_change_needs_no_bump(self):
        self.assertFalse(fh.needs_bump(header(status="@status      stable") + "x\n",
                                       header() + "x\n"))

    def test_new_file_needs_no_bump(self):
        self.assertFalse(fh.needs_bump(header(), None))


class HookTest(unittest.TestCase):
    """Runs real commits through the installed hook."""
    OLD = "2026-02-01"
    TODAY = fh.date.today().isoformat()

    def setUp(self):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.root = Path(directory.name).resolve()
        self.git("init", "-q", "-b", "main")
        self.git("config", "user.email", "test@example.invalid")
        self.git("config", "user.name", "Test")
        (self.root / fh.CONFIG_NAME).write_text('[projects]\nlib = "Nexus Library"\n')
        (self.root / "lib").mkdir()
        self.write("lib/a.hpp", header() + "int a;\n")
        self.git("add", "-A")
        self.git("commit", "-qm", "Feat: add")
        self.tool("--install-hook")

    def git(self, *args, check=True):
        return subprocess.run(["git", "-C", str(self.root), *args], check=check,
                              capture_output=True, text=True)

    def tool(self, *args):
        script = Path(fh.__file__).resolve()
        return subprocess.run([str(script), *args], cwd=self.root, check=True,
                              capture_output=True, text=True)

    def write(self, path, text):
        (self.root / path).write_text(text)

    def read(self, path):
        return (self.root / path).read_text()

    def committed(self, path="lib/a.hpp"):
        return self.git("show", f"HEAD:{path}").stdout

    def status(self):
        return self.git("status", "--porcelain").stdout

    def test_code_change_bumps_commit_and_working_tree(self):
        self.write("lib/a.hpp", header() + "int b;\n")
        self.git("add", "-A")
        self.git("commit", "-qm", "Feat: change")
        self.assertIn(f"@updated     {self.TODAY}", self.committed())
        self.assertIn(f"@updated     {self.TODAY}", self.read("lib/a.hpp"))
        self.assertEqual(self.status(), "")

    def test_header_only_change_keeps_the_date(self):
        self.write("lib/a.hpp", header(reviewed="@reviewed    skimmed") + "int a;\n")
        self.git("add", "-A")
        self.git("commit", "-qm", "Misc: review")
        self.assertIn(f"@updated     {self.OLD}", self.committed())

    def test_unstaged_changes_stay_out_of_the_commit(self):
        self.write("lib/a.hpp", header() + "int staged;\n")
        self.git("add", "-A")
        self.write("lib/a.hpp", header() + "int staged;\nint unstaged;\n")
        self.git("commit", "-qm", "Feat: part")
        self.assertNotIn("unstaged", self.committed())
        self.assertIn(f"@updated     {self.TODAY}", self.committed())
        self.assertIn("int unstaged;", self.read("lib/a.hpp"))
        self.assertIn(f"@updated     {self.TODAY}", self.read("lib/a.hpp"))
        self.assertEqual(self.status(), " M lib/a.hpp\n")

    def test_commit_all(self):
        self.write("lib/a.hpp", header() + "int b;\n")
        self.git("commit", "-qam", "Feat: all")
        self.assertIn(f"@updated     {self.TODAY}", self.committed())
        self.assertEqual(self.status(), "")

    def test_commit_of_named_paths(self):
        self.write("lib/a.hpp", header() + "int b;\n")
        self.git("commit", "-qm", "Feat: path", "lib/a.hpp")
        self.assertIn(f"@updated     {self.TODAY}", self.committed())

        # Git locks the real index during such a commit: it catches up on the next add.
        self.assertEqual(self.status(), "MM lib/a.hpp\n")
        self.git("add", "lib/a.hpp")
        self.assertEqual(self.status(), "")

    def test_errors_stop_the_commit(self):
        self.write("lib/a.hpp", header(status="@status      bogus") + "int a;\n")
        self.git("add", "-A")
        result = self.git("commit", "-qm", "Feat: bad", check=False)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("unknown @status", result.stderr)

    def test_staged_content_is_what_gets_checked(self):
        self.write("lib/a.hpp", header(status="@status      bogus") + "int a;\n")
        self.git("add", "-A")
        self.write("lib/a.hpp", header() + "int a;\n")
        self.assertNotEqual(self.git("commit", "-qm", "Feat: x", check=False).returncode, 0)

    def test_unrelated_broken_files_do_not_stop_the_commit(self):
        self.write("lib/broken.hpp", "int x;\n")
        self.git("add", "lib/broken.hpp")
        self.git("commit", "-qm", "Feat: broken", "--no-verify")
        self.write("lib/a.hpp", header() + "int b;\n")
        self.git("add", "lib/a.hpp")
        self.git("commit", "-qm", "Feat: fine")

    def test_non_source_files_are_ignored(self):
        self.write("notes.txt", "hello\n")
        self.git("add", "-A")
        self.git("commit", "-qm", "Misc: notes")

    def test_new_file_keeps_its_date(self):
        self.write("lib/b.hpp", header() + "int b;\n")
        self.git("add", "-A")
        self.git("commit", "-qm", "Feat: new")
        self.assertIn(f"@updated     {self.OLD}", self.committed("lib/b.hpp"))

    def test_pure_rename_keeps_the_date(self):
        self.git("mv", "lib/a.hpp", "lib/b.hpp")
        self.git("commit", "-qm", "Refact: move")
        self.assertIn(f"@updated     {self.OLD}", self.committed("lib/b.hpp"))

    def test_rename_with_changes_bumps(self):
        text = header() + "".join(f"int line{index};\n" for index in range(40))
        self.write("lib/a.hpp", text)
        self.git("add", "-A")
        self.git("commit", "-qm", "Feat: grow", "--no-verify")
        self.git("mv", "lib/a.hpp", "lib/b.hpp")
        self.write("lib/b.hpp", text + "int more;\n")
        self.git("add", "-A")
        self.git("commit", "-qm", "Refact: move")
        self.assertIn(f"@updated     {self.TODAY}", self.committed("lib/b.hpp"))

    def test_no_bump_variable_keeps_the_date(self):
        self.write("lib/a.hpp", header() + "int b;\n")
        self.git("add", "-A")
        subprocess.run(["git", "-C", str(self.root), "commit", "-qm", "Misc: reformat"], check=True,
                       env=os.environ | {fh.NO_BUMP_VARIABLE: "1"})
        self.assertIn(f"@updated     {self.OLD}", self.committed())

    def test_merge_keeps_the_dates(self):
        self.git("checkout", "-qb", "side")
        self.write("lib/a.hpp", header() + "int a;\nint side;\n")
        self.git("commit", "-qam", "Feat: side", "--no-verify")
        self.git("checkout", "-q", "main")
        self.write("lib/b.hpp", header() + "int b;\n")
        self.git("add", "-A")
        self.git("commit", "-qm", "Feat: main")
        self.git("merge", "-q", "--no-ff", "-m", "Merge", "side")
        self.assertIn(f"@updated     {self.OLD}", self.committed())

    def test_crlf_header_change_keeps_the_date(self):
        text = header().replace("\n", "\r\n") + "int a;\r\n"
        self.write("lib/c.hpp", text)
        self.git("add", "-A")
        self.git("commit", "-qm", "Feat: crlf")
        (self.root / "lib/c.hpp").write_bytes(
            text.replace("@reviewed    none", "@reviewed    read").encode())
        self.git("add", "-A")
        self.git("commit", "-qm", "Misc: review")
        self.assertIn(f"@updated     {self.OLD}", self.committed("lib/c.hpp"))

    def test_foreign_hook_is_not_overwritten(self):
        hook = self.root / ".git/hooks/pre-commit"
        hook.write_text("#!/bin/sh\nexit 0\n")
        script = Path(fh.__file__).resolve()
        result = subprocess.run([str(script), "--install-hook"], cwd=self.root,
                                capture_output=True, text=True, check=False)
        self.assertEqual(result.returncode, 2)
        self.assertEqual(hook.read_text(), "#!/bin/sh\nexit 0\n")

    def test_installing_twice_is_fine(self):
        self.tool("--install-hook")


class TargetTest(unittest.TestCase):
    def setUp(self):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.base = Path(directory.name).resolve()
        for name in ("one", "two"):
            (self.base / name / "lib").mkdir(parents=True)
            subprocess.run(["git", "-C", str(self.base / name), "init", "-q"], check=True)
            (self.base / name / "lib/a.hpp").touch()

    def test_repository_comes_from_the_path(self):
        root, selection = fh.resolve_targets([str(self.base / "two/lib/a.hpp")])
        self.assertEqual((root, selection), (self.base / "two", ["lib/a.hpp"]))

    def test_repository_root_selects_everything(self):
        root, selection = fh.resolve_targets([f"{self.base / 'two'}/"])
        self.assertEqual((root, selection), (self.base / "two", []))

    def test_paths_are_relative_to_the_working_directory(self):
        previous = Path.cwd()
        self.addCleanup(os.chdir, previous)
        os.chdir(self.base / "one/lib")
        self.assertEqual(fh.resolve_targets(["a.hpp"]), (self.base / "one", ["lib/a.hpp"]))

    def test_missing_path(self):
        with self.assertRaisesRegex(ValueError, "no such file"):
            fh.resolve_targets([str(self.base / "one/nope.hpp")])

    def test_several_repositories(self):
        with self.assertRaisesRegex(ValueError, "several repositories"):
            fh.resolve_targets([str(self.base / "one"), str(self.base / "two")])

    def test_outside_a_repository(self):
        with self.assertRaisesRegex(ValueError, "not inside a git repository"):
            fh.resolve_targets([str(self.base)])


if __name__ == "__main__":
    unittest.main()
