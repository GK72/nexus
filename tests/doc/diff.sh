#!/usr/bin/env bash
#
# Part of Nexus Library
#
# Function tests of `tools/doc.sh --diff` on a small book in a fixture repository: what each kind of
# change (unstaged, staged, untracked, deleted) marks in the rendered pages, and the cases without
# marks. Hermetic: no global git config.
#
#   tests/doc/diff.sh

# shellcheck disable=SC2329  # the cases are called through tap::case
set -euo pipefail
# shellcheck source=../../tools/ci/nci-test.sh
source "$(dirname "$(realpath "$0")")/../../tools/ci/nci-test.sh"

DOC="${TAP_ROOT}/tools/doc.sh"
command -v asciidoctor >/dev/null || { echo "Bail out! asciidoctor not found"; exit 1; }
ruby -e 'require "nokogiri"' 2>/dev/null || { echo "Bail out! nokogiri not found (gem install nokogiri)"; exit 1; }

export GIT_CONFIG_GLOBAL=/dev/null GIT_CONFIG_NOSYSTEM=1
export GIT_AUTHOR_NAME=test GIT_AUTHOR_EMAIL=test@example.com
export GIT_COMMITTER_NAME=test GIT_COMMITTER_EMAIL=test@example.com

# repo/doc/
#   book.adoc        the spine: includes alpha, beta and gone
#   pages/alpha.adoc a paragraph, a list and a table
#   pages/beta.adoc  a paragraph
#   pages/gone.adoc  a paragraph
make_book() {
    local -r dir="$1"
    mkdir -p "${dir}/doc/pages"
    cat > "${dir}/doc/book.adoc" <<'EOF'
= Book
:toc: left

include::pages/alpha.adoc[leveloffset=+1]
include::pages/beta.adoc[leveloffset=+1]
include::pages/gone.adoc[leveloffset=+1]
EOF
    cat > "${dir}/doc/pages/alpha.adoc" <<'EOF'
= Alpha

The edge runs the reducer shards and the outbox,
and nothing else.

* first item
* second item

.Roles.
|===
| Role | Runs

| edge | ingest, the outbox
| global | the gateway
|===
EOF
    printf '= Beta\n\nBeta has one paragraph.\n' > "${dir}/doc/pages/beta.adoc"
    printf '= Gone\n\nThis page goes away.\n' > "${dir}/doc/pages/gone.adoc"
}

# A fresh repository with the book committed, in the case's folder; prints its path
fixture() {
    local -r repo="${TAP_CASE_DIR}/repo"
    make_book "${repo}"
    git -C "${repo}" init -q -b main
    git -C "${repo}" add -A
    git -C "${repo}" commit -qm book
    printf '%s\n' "${repo}"
}

render() {  # render REPO [OPTION...]: the book to REPO/out
    local -r repo="$1"
    shift
    tap::run "${DOC}" -o "${repo}/out" "$@" "${repo}/doc"
    tap::expect_status
}

count() {  # count FILE KIND: the changes of KIND marked in FILE
    grep -o "data-nx-change=\"$2\"" "$1" | wc -l
}

expect_untouched() {  # expect_untouched FILE...: no marks, no toolbar
    local file
    for file in "$@"; do
        ! grep -q 'nx-diff' "${file}" || tap::_fail "$(basename "${file}") has diff marks or the toolbar"
    done
}

expect_no_marks() {  # expect_no_marks FILE...: no marks (the toolbar may list the changed pages)
    local file
    for file in "$@"; do
        ! grep -qE 'data-nx-change="|nx-diff-badge"' "${file}" || tap::_fail "$(basename "${file}") has diff marks"
    done
}

without_flag_no_marks() {
    local repo
    repo="$(fixture)"
    printf 'Changed paragraph.\n' >> "${repo}/doc/pages/beta.adoc"
    render "${repo}"
    expect_untouched "${repo}"/out/book/*.html
}

no_change_no_marks() {
    local repo
    repo="$(fixture)"
    render "${repo}" --diff
    expect_untouched "${repo}"/out/book/*.html
    tap::expect_eq "$(wc -l < "${TAP_STDERR}")" 0 "lines on stderr"
}

outside_git_no_marks() {
    # Outside the repository of the tests, which holds TAP_DIR
    local dir
    dir="$(mktemp -d)"
    # shellcheck disable=SC2064  # the folder now: the trap runs when the case's subshell ends
    trap "rm -rf '${dir}'" EXIT
    make_book "${dir}"
    render "${dir}" --diff
    expect_untouched "${dir}"/out/book/*.html
    tap::expect_contains "${TAP_STDERR}" "not in a git repository"
}

ignored_book_no_marks() {
    local repo
    repo="$(fixture)"
    make_book "${repo}/build/copy"
    printf 'build/\n' > "${repo}/.gitignore"
    render "${repo}/build/copy" --diff
    expect_untouched "${repo}"/build/copy/out/book/*.html
}

unstaged_edit_marks_words() {
    local repo
    repo="$(fixture)"
    sed -i 's/reducer shards and the outbox,/outbox,/' "${repo}/doc/pages/alpha.adoc"
    render "${repo}" --diff
    local -r page="${repo}/out/book/alpha.html"
    tap::expect_eq "$(count "${page}" changed)" 1 "changes in alpha"
    tap::expect_contains "${page}" '<del class="nx-del">reducer shards and the</del>' 'data-nx-diff-ref="HEAD"'
    tap::expect_eq "$(count "${repo}/out/book/index.html" changed)" 1 "changes in the spine"
    expect_no_marks "${repo}/out/book/beta.html"
}

staged_and_unstaged_both_count() {
    local repo
    repo="$(fixture)"
    sed -i 's/| global | the gateway/| global | the gateway, the exporter/' "${repo}/doc/pages/alpha.adoc"
    git -C "${repo}" add -A
    printf '\nA staged page gets an unstaged paragraph.\n' >> "${repo}/doc/pages/alpha.adoc"
    render "${repo}" --diff
    local -r page="${repo}/out/book/alpha.html"
    tap::expect_eq "$(count "${page}" changed)" 1 "changed rows"
    tap::expect_eq "$(count "${page}" added)" 1 "added paragraphs"
    tap::expect_contains "${page}" '<ins class="nx-ins">, the exporter</ins>'
}

untracked_page_is_new() {
    local repo
    repo="$(fixture)"
    printf '= Fresh\n\nAll of it is new.\n' > "${repo}/doc/pages/fresh.adoc"
    sed -i 's|^include::pages/beta.adoc.*|&\ninclude::pages/fresh.adoc[leveloffset=+1]|' "${repo}/doc/book.adoc"
    render "${repo}" --diff
    tap::expect_contains "${repo}/out/book/fresh.html" 'data-nx-diff-new-page="true"' '>New page</span>'
    tap::expect_eq "$(count "${repo}/out/book/fresh.html" added)" 0 "marks in the new page"
    tap::expect_eq "$(count "${repo}/out/book/index.html" added)" 1 "new sections in the spine"
    tap::expect_match "${repo}/out/book/index.html" 'class="sect1 nx-diff nx-diff-added"'
    tap::expect_contains "${repo}/out/book/beta.html" 'data-nx-diff-pages="index fresh"'
}

deleted_page_shows_struck() {
    local repo
    repo="$(fixture)"
    git -C "${repo}" rm -q doc/pages/gone.adoc
    sed -i '/gone.adoc/d' "${repo}/doc/book.adoc"
    render "${repo}" --diff
    tap::expect_eq "$(count "${repo}/out/book/index.html" deleted)" 1 "deleted sections in the spine"
    tap::expect_contains "${repo}/out/book/index.html" 'This page goes away.' '>Deleted</span>'
    [[ ! -e "${repo}/out/book/gone.html" ]] || tap::_fail "gone.html rendered"
}

rewrap_is_no_change() {
    local repo
    repo="$(fixture)"
    sed -i -e 's/reducer shards and the outbox,/reducer shards/' -e 's/^and nothing else\./and the outbox, and nothing else./' \
        "${repo}/doc/pages/alpha.adoc"
    render "${repo}" --diff
    expect_no_marks "${repo}/out/book/alpha.html"
}

older_ref() {
    local repo
    repo="$(fixture)"
    printf '\nA second commit.\n' >> "${repo}/doc/pages/beta.adoc"
    git -C "${repo}" commit -qam second
    render "${repo}" --diff=HEAD~1
    tap::expect_eq "$(count "${repo}/out/book/beta.html" added)" 1 "added since HEAD~1"
    tap::expect_contains "${repo}/out/book/beta.html" 'data-nx-diff-ref="HEAD~1"'
}

unknown_ref_fails() {
    local repo
    repo="$(fixture)"
    tap::run "${DOC}" -o "${repo}/out" --diff=no-such-ref "${repo}/doc"
    tap::expect_fail
    tap::expect_contains "${TAP_STDERR}" "no such commit"
}

tap::case "without --diff, a changed book has no marks" without_flag_no_marks
tap::case "--diff without a change has no marks" no_change_no_marks
tap::case "--diff outside git renders without marks" outside_git_no_marks
tap::case "--diff on a book that git ignores has no marks" ignored_book_no_marks
tap::case "an unstaged edit marks its words" unstaged_edit_marks_words
tap::case "staged and unstaged edits both count" staged_and_unstaged_both_count
tap::case "an untracked page is new" untracked_page_is_new
tap::case "a deleted page is struck in the spine" deleted_page_shows_struck
tap::case "a rewrapped paragraph is no change" rewrap_is_no_change
tap::case "--diff=REF compares with an older commit" older_ref
tap::case "--diff with an unknown commit fails" unknown_ref_fails
tap::done
