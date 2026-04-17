#!/usr/bin/env bash
set -euo pipefail

repo_root="$(git rev-parse --show-toplevel)"
cd "$repo_root"

tag_pattern="${1:-\\[[^]]*plan[^]]*\\]}"

last_commit_line="$(git log --format='%H%x09%h%x09%s' --extended-regexp --grep="$tag_pattern" -n 1)"

if [[ -z "$last_commit_line" ]]; then
  printf 'LAST_PLAN_CHANGE_MISSING\n'
  exit 1
fi

full_sha="${last_commit_line%%$'\t'*}"
rest="${last_commit_line#*$'\t'}"
short_sha="${rest%%$'\t'*}"
subject="${rest#*$'\t'}"
gap="$(git rev-list --count "${full_sha}..HEAD")"

printf 'last_plan_change_sha=%s\n' "$short_sha"
printf 'last_plan_change_gap=%s\n' "$gap"
printf 'last_plan_change_subject=%s\n' "$subject"
