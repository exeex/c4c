# Active Plan Todo

Status: Active
Source Idea: ideas/open/02_stmt_emitter_cpp_split_refactor.md
Source Plan: plan.md

## Current Active Item

- Step 1: survey the emitter monolith and the existing header surface
- Current slice: inspect `src/codegen/lir/stmt_emitter.cpp` and `src/codegen/lir/stmt_emitter.hpp`, then record a first-pass ownership map for the draft split files without changing implementation code yet

## Todo

- [ ] Step 1: survey `stmt_emitter.cpp` and `stmt_emitter.hpp`
- [ ] Step 2: create draft staging files for the first semantic clusters
- [ ] Step 3: expand the draft set and normalize boundaries
- [ ] Step 4: wire the split files into the build
- [ ] Step 5: reduce the monolith cleanly
- [ ] Step 6: run final build and regression validation

## Completed

- Activated from [`ideas/open/02_stmt_emitter_cpp_split_refactor.md`](/workspaces/c4c/ideas/open/02_stmt_emitter_cpp_split_refactor.md) on 2026-04-02 after closing the completed `ast_to_hir.cpp` split refactor.

## Notes

- The idea explicitly prefers a draft-first extraction pass before build wiring; the initial execution slice should respect that guardrail.
