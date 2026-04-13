#!/usr/bin/env bash
set -euo pipefail

repo_root="$(git rev-parse --show-toplevel)"
cd "$repo_root"

has_plan_change=0
has_todo_change=0
has_idea_open_change=0

while IFS= read -r path; do
  case "$path" in
    plan.md)
      has_plan_change=1
      ;;
    todo.md)
      has_todo_change=1
      ;;
    ideas/open/*)
      has_idea_open_change=1
      ;;
  esac
done < <(git diff --cached --name-only --diff-filter=ACMR)

tags=()
if [[ "$has_plan_change" -eq 1 ]]; then
  tags+=("[plan_change]")
fi
if [[ "$has_todo_change" -eq 1 ]]; then
  tags+=("[todo_change]")
fi
if [[ "$has_idea_open_change" -eq 1 ]]; then
  tags+=("[idea_open_change]")
fi

printf '%s\n' "${tags[*]-}"
