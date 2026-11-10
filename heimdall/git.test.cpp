/**
 * Part of Nexus Library
 *
 * @brief   Tests of the parsers of Git's output.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-11-10
 * @updated     2026-11-10
 * @reviewed    none
 * SPDX-License-Identifier: Unlicense
 */

#include "heimdall/git.hpp"

#include <gtest/gtest.h>

#include <string>

using namespace std::string_literals;

TEST(Git, ParseWorktrees_MainLinkedAndDetached) {
    const auto trees = heimdall::parse_worktrees(
        "worktree /r/app\nHEAD 0123456789abcdef\nbranch refs/heads/main\n\n"
        "worktree /r/app/wt\nHEAD fedcba9876543210\nbranch refs/heads/feature\nlocked\n\n"
        "worktree /r/app/det\nHEAD 1111111222222\ndetached\n\n");
    ASSERT_EQ(trees.size(), 3);
    EXPECT_TRUE(trees[0].main);
    EXPECT_EQ(trees[0].branch, "main");
    EXPECT_EQ(trees[0].head, "0123456");
    EXPECT_FALSE(trees[1].main);
    EXPECT_TRUE(trees[1].locked);
    EXPECT_EQ(trees[2].branch, std::nullopt);
}

TEST(Git, ParseWorktrees_SkipsBareAndPrunable) {
    const auto trees = heimdall::parse_worktrees(
        "worktree /r/app.git\nbare\n\n"
        "worktree /r/gone\nHEAD 0123456789\nbranch refs/heads/x\nprunable gitdir file points to non-existent location\n\n"
        "worktree /r/wt\nHEAD 0123456789\nbranch refs/heads/y\n");
    ASSERT_EQ(trees.size(), 1);
    EXPECT_EQ(trees[0].path, "/r/wt");
    EXPECT_FALSE(trees[0].main);
}

TEST(Git, ParseStatus_CountsChangesAndTracking) {
    const auto status = heimdall::parse_status(
        "# branch.oid abc\n# branch.head main\n# branch.upstream origin/main\n# branch.ab +2 -3\n"
        "1 M. N... 100644 100644 100644 a b file1\n"
        "1 .M N... 100644 100644 100644 a b file2\n"
        "1 MM N... 100644 100644 100644 a b file3\n"
        "2 R. N... 100644 100644 100644 a b R100 new\told\n"
        "u UU N... 100644 100644 100644 100644 a b c conflict\n"
        "? new1\n? new2\n");
    EXPECT_EQ(status.staged, 3);
    EXPECT_EQ(status.modified, 2);
    EXPECT_EQ(status.conflicts, 1);
    EXPECT_EQ(status.untracked, 2);
    EXPECT_EQ(status.ahead, 2);
    EXPECT_EQ(status.behind, 3);
    EXPECT_EQ(status.upstream, "origin/main");
    EXPECT_TRUE(status.dirty());
}

TEST(Git, ParseStatus_CleanWithoutUpstream) {
    const auto status = heimdall::parse_status("# branch.oid abc\n# branch.head main\n");
    EXPECT_FALSE(status.dirty());
    EXPECT_EQ(status.upstream, std::nullopt);
}

TEST(Git, ParseBranches_TrackAndGone) {
    const auto text =
        "main\0abc1234\0001700000000\0origin/main\0behind 2\0/r/app\0Initial\n"s
        "feature\0def5678\0001700000100\0origin/feature\0ahead 1, behind 4\0\0Work\n"s
        "old\0aaa1111\0001600000000\0origin/old\0gone\0\0Old work\n"s
        "local\0bbb2222\0001700000200\0\0\0\0Local\n"s;
    const auto branches = heimdall::parse_branches(text);
    ASSERT_EQ(branches.size(), 4);
    EXPECT_EQ(branches[0].behind, 2);
    EXPECT_EQ(branches[0].worktree, "/r/app");
    EXPECT_EQ(branches[0].committed, 1700000000);
    EXPECT_EQ(branches[1].ahead, 1);
    EXPECT_EQ(branches[1].behind, 4);
    EXPECT_TRUE(branches[2].gone);
    EXPECT_TRUE(branches[2].notable());
    EXPECT_EQ(branches[3].upstream, std::nullopt);
    EXPECT_FALSE(branches[3].notable());
}

TEST(Git, ParseStashes) {
    const auto stashes = heimdall::parse_stashes("stash@{0}\0001700000000\0WIP on main: abc Thing\n"s);
    ASSERT_EQ(stashes.size(), 1);
    EXPECT_EQ(stashes[0].ref, "stash@{0}");
    EXPECT_EQ(stashes[0].created, 1700000000);
    EXPECT_EQ(stashes[0].message, "WIP on main: abc Thing");
}

TEST(Git, FetchReason_PrefersTheCause) {
    EXPECT_EQ(heimdall::fetch_reason(
        "git@github.com: Permission denied (publickey).\n"
        "fatal: Could not read from remote repository.\n\n"
        "Please make sure you have the correct access rights\nand the repository exists.\n"),
        "git@github.com: Permission denied (publickey).");
    EXPECT_EQ(heimdall::fetch_reason("fatal: unable to access 'x'\n"), "fatal: unable to access 'x'");
    EXPECT_EQ(heimdall::fetch_reason(""), "");
}

TEST(Git, Attention) {
    heimdall::repo_t repo;
    EXPECT_FALSE(repo.attention());
    repo.stashes.push_back({});
    EXPECT_TRUE(repo.attention());
    repo.stashes.clear();
    repo.branches.push_back({ .name = "x", .gone = true });
    EXPECT_TRUE(repo.attention());
    EXPECT_EQ(repo.gone(), 1);
}
