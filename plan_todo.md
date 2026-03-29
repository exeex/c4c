# Backend Port Roadmap Todo

Status: Active
Source Idea: ideas/open/__backend_port_plan.md
Source Plan: plan.md

## Current Active Item

- [ ] Step 1: Audit the remaining backend roadmap queue
- Iteration slice: compare the umbrella roadmap against the current
  `ideas/open/` and `ideas/closed/` inventory after closing the bounded x86
  linker plan, then pick the narrowest viable next child idea to restore the
  backend execution queue.

## Planned Items

- [ ] Step 1: Audit the remaining backend roadmap queue
- [ ] Step 2: Write the next bounded child idea
- [ ] Step 3: Switch lifecycle state to the new child idea

## Completed Items

- [x] Activated `ideas/open/__backend_port_plan.md` into the active runbook
  after closing `ideas/closed/24_backend_builtin_linker_x86_plan.md`

## Next Intended Slice

- Inventory the remaining roadmap entries against the current closed/open idea
  set.
- Select one bounded backend slice that should become a new child idea.
- Avoid direct implementation from the umbrella roadmap.

## Blockers

- The umbrella roadmap is the only remaining open idea, so execution must first
  recreate a narrower child idea before normal backend implementation can
  resume.

## Resume Notes

- The first built-in x86 linker slice is closed at
  `ideas/closed/24_backend_builtin_linker_x86_plan.md`.
- The umbrella roadmap should not stay active longer than needed to restore a
  concrete child-idea queue.
