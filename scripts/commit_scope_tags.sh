#!/usr/bin/env bash
set -euo pipefail

repo_root="$(git rev-parse --show-toplevel)"
cd "$repo_root"

has_plan_change=0
has_todo_change=0
has_idea_open_change=0
staged_path_count=0

while IFS= read -r path; do
  [[ -z "$path" ]] && continue
  staged_path_count=$((staged_path_count + 1))
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

components=()
if [[ "$has_plan_change" -eq 1 ]]; then
  components+=("plan")
fi
if [[ "$has_idea_open_change" -eq 1 ]]; then
  components+=("idea")
fi
if [[ "$has_todo_change" -eq 1 && "$staged_path_count" -eq 1 ]]; then
  components+=("todo_only")
fi

if [[ "${#components[@]}" -eq 0 ]]; then
  exit 0
fi

joined_tag="["
for i in "${!components[@]}"; do
  if [[ "$i" -gt 0 ]]; then
    joined_tag+="+"
  fi
  joined_tag+="${components[$i]}"
done
joined_tag+="]"

printf '%s\n' "$joined_tag"
