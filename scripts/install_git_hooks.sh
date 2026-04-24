#!/usr/bin/env bash
set -euo pipefail

repo_root="$(git rev-parse --show-toplevel)"
cd "$repo_root"

chmod +x scripts/commit_scope_tags.sh scripts/plan_review_state.py \
  .githooks/pre-commit .githooks/post-commit \
  .githooks/prepare-commit-msg .githooks/commit-msg
git config core.hooksPath .githooks

printf 'Installed repo hooks with core.hooksPath=.githooks\n'
