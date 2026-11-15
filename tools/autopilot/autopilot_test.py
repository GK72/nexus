#!/usr/bin/env python3
#
# Part of Nexus Library
#
# Unit tests of autopilot's pure policy: budget line, budget check, picking, surveys, config.

import unittest

from autopilot import (DEFAULTS, WINDOW, PickContext, Usage, budget_check, dependents, five_hour_line, merge_config,
                       pick, survey_due, surveyable)

BUDGET = dict(five_hour_start=50, five_hour_end=90, seven_day=80)
NOW = 1_000_000.0


def task(id_, kind='feature', priority=2, hours=1.0, state='ready'):
    return dict(id=id_, project=id_.split('/')[0], kind=kind, priority=priority, current=hours,
                state=state, approved=True)


def context(**changes):
    values = dict(projects={'a', 'b'}, exclude=[], attempted=set(), dependents={}, recent_kinds=[],
                  debt_share=0.4)
    values.update(changes)
    return PickContext(**values)


class BudgetTest(unittest.TestCase):
    def test_line_rises_over_the_window(self):
        self.assertEqual(five_hour_line(NOW, NOW + WINDOW, 50, 90), 50)
        self.assertEqual(five_hour_line(NOW, NOW + WINDOW / 2, 50, 90), 70)
        self.assertEqual(five_hour_line(NOW, NOW, 50, 90), 90)

    def test_under_and_over_the_line(self):
        self.assertTrue(budget_check(Usage(40, NOW + WINDOW, 10, NOW + 1), NOW, BUDGET)[0])
        self.assertFalse(budget_check(Usage(55, NOW + WINDOW, 10, NOW + 1), NOW, BUDGET)[0])
        self.assertTrue(budget_check(Usage(55, NOW + 600, 10, NOW + 1), NOW, BUDGET)[0])

    def test_weekly_limit(self):
        ok, reason = budget_check(Usage(10, NOW + WINDOW, 85, NOW + 1), NOW, BUDGET)
        self.assertFalse(ok)
        self.assertIn('weekly', reason)

    def test_reset_window_counts_as_empty(self):
        self.assertTrue(budget_check(Usage(99, NOW - 1, 99, NOW - 1), NOW, BUDGET)[0])


class PickTest(unittest.TestCase):
    def test_priority_wins(self):
        tasks = [task('a/x/low', priority=3), task('a/x/high', priority=1)]
        self.assertEqual(pick(tasks, context())['id'], 'a/x/high')

    def test_filters(self):
        tasks = [task('c/x/other-project'), task('a/x/review', kind='review'), task('a/x/blocked', state='blocked'),
                 task('a/x/tried'), task('a/skip/me'), task('a/x/proposed', state='proposed')]
        self.assertIsNone(pick(tasks, context(attempted={'a/x/tried'}, exclude=['a/skip/*'])))

    def test_debt_balance(self):
        tasks = [task('a/x/feature'), task('a/x/debt', kind='debt')]
        self.assertEqual(pick(tasks, context(recent_kinds=['feature'] * 5))['id'], 'a/x/debt')
        self.assertEqual(pick(tasks, context(recent_kinds=['debt'] * 5))['id'], 'a/x/feature')

    def test_unblocking_bonus(self):
        tasks = [task('a/x/plain'), task('a/x/unblocks')]
        self.assertEqual(pick(tasks, context(dependents={'a/x/unblocks': 2}))['id'], 'a/x/unblocks')

    def test_small_tasks_when_budget_is_tight(self):
        tasks = [task('a/x/big', hours=4), task('a/x/small', hours=0.5)]
        self.assertEqual(pick(tasks, context(headroom=5))['id'], 'a/x/small')

    def test_dependents(self):
        deps = [dict(task='a/1', needs='a/0'), dict(task='a/2', needs='a/0')]
        self.assertEqual(dependents(deps), {'a/0': 2})


class SurveyTest(unittest.TestCase):
    def test_oldest_due_first(self):
        surveys = {'a': NOW - 10 * 86400, 'b': NOW - 20 * 86400, 'c': NOW}
        self.assertEqual(survey_due(['a', 'b', 'c'], surveys, NOW, 7), 'b')
        self.assertIsNone(survey_due(['c'], surveys, NOW, 7))
        self.assertEqual(survey_due(['d'], surveys, NOW, 7), 'd')

    def test_shelved_and_opted_out_are_not_surveyed(self):
        config = {'a': {}, 'b': {'survey': False}, 'c': {}, 'new': {}}
        norn = {'a': {'shelved': False, 'owner': 'autopilot'}, 'c': {'shelved': True, 'owner': 'author'}}
        self.assertEqual(surveyable(config, norn), ['a', 'new'])

    def test_unapproved_task_ready_in_owned_project_is_taken(self):
        self.assertEqual(pick([dict(task('a/x/mine'), approved=False)], context())['id'], 'a/x/mine')


class ConfigTest(unittest.TestCase):
    PROJECTS = {'projects': {'a': {'repo': '~/repos/a', 'branch': 'main'}}}

    def test_defaults_with_projects(self):
        config = merge_config(self.PROJECTS)
        self.assertEqual(config['budget'], DEFAULTS['budget'])
        self.assertEqual(config['projects']['a']['branch'], 'main')

    def test_override_key_by_key(self):
        config = merge_config({**self.PROJECTS, 'budget': {'seven_day': 60}, 'exclude': ['a/x/*']})
        self.assertEqual(config['budget']['seven_day'], 60)
        self.assertEqual(config['budget']['five_hour_start'], 50)
        self.assertEqual(config['exclude'], ['a/x/*'])
        self.assertEqual(DEFAULTS['budget']['seven_day'], 80)

    def test_rejected(self):
        for bad in ({}, {**self.PROJECTS, 'budjet': {}}, {**self.PROJECTS, 'budget': {'seven_days': 1}},
                    {'projects': {'a': {'repo': 'r'}}}, {'projects': {'a': {'repo': 'r', 'branch': 'b', 'gat': ''}}},
                    {'projects': {'a': {'repo': 'r', 'branch': 'b', 'worktree': 'w'}}}):
            with self.subTest(bad=bad), self.assertRaises(ValueError):
                merge_config(bad)


if __name__ == '__main__':
    unittest.main()
