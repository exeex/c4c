# Parser Whitelist Boundary Audit Todo

Status: Active
Source Idea: ideas/open/07_parser_whitelist_boundary_audit.md
Source Plan: plan.md

## Current Active Item

- [ ] Step 3: Reduce the older `src/frontend/parser/types.cpp` duplicate
  C++20 `requires` / constraint boundary helper so it no longer inherits the
  broad `is_type_start()` stop condition

## Checklist

- [x] Step 1: Build the audit inventory
- [x] Step 2: Classify breadth and risk
- [ ] Step 3: Reduce the first tightening slice
- [ ] Step 4: Tighten the boundary
- [ ] Step 5: Validate and record the next slice

## Completed

- [x] Activated `ideas/open/07_parser_whitelist_boundary_audit.md` into
  `plan.md`
- [x] Reset `plan_todo.md` for the active runbook
- [x] Recorded the Step 1 / Step 2 audit inventory and risk tags in
  `src/frontend/parser/BOUNDARY_AUDIT.md`
- [x] Landed earlier top-level discard tightenings already reflected in
  `src/frontend/parser/BOUNDARY_AUDIT.md`; the next remaining slice is a
  forward tag declaration cleanup
- [x] Tightened top-level forward tag declarations in
  `src/frontend/parser/declarations.cpp` so bookkeeping-only declarations such
  as `struct Forward;` no longer materialize a synthetic `NK_EMPTY` node
- [x] Added the reduced parse-only regression
  `tests/cpp/internal/parse_only_case/top_level_forward_tag_decl_preserves_following_decl_parse.cpp`
  plus the dedicated dump assertion
  `cpp_parse_top_level_forward_tag_decl_preserves_following_decl_dump` in
  `tests/cpp/internal/InternalTests.cmake`
- [x] Re-ran focused top-level parse coverage and the full regression guard:
  baseline `2424 passed / 1 failed / 2425 total`, after `2425 passed / 1 failed / 2426 total`,
  no newly failing tests
- [x] Tightened the generic unsupported top-level declaration recovery in
  `src/frontend/parser/declarations.cpp` so malformed lines that stop at the
  next declaration boundary no longer materialize a synthetic `NK_EMPTY` node
- [x] Strengthened the reduced parse-only assertions
  `cpp_parse_top_level_class_recovery_preserves_following_decl_dump` and
  `cpp_parse_top_level_storage_class_recovery_preserves_following_decl_dump`
  to fail on `Empty`, and re-ran the full top-level parse-only dump coverage
- [x] Re-ran the full regression guard after the unsupported-declaration
  tightening: baseline `2424 passed / 1 failed / 2425 total`, after
  `2425 passed / 1 failed / 2426 total`, no newly failing tests

## Next Intended Slice

Inspect the older `types.cpp` copies of the C++20 `requires` boundary helpers,
starting with `skip_cpp20_constraint_atom(Parser&)`, and reduce one case that
shows the broad `is_type_start()`-driven stop rule still accepts too much input
before a following declaration boundary.

## Blockers

- None recorded at activation time.

## Resume Notes

- `ideas/open/04_std_vector_bringup_plan.md` is parked and points to this audit
  as the preferred next dependency.
- Keep this runbook narrow; adjacent builtin-trait and SFINAE work already have
  separate open ideas.
- `src/frontend/parser/BOUNDARY_AUDIT.md` is the durable Step 1 / Step 2
  inventory; `plan_todo.md` should only track the current tightening slice.
- The just-finished reduced slice was the generic `top_level_decl_recovery_done`
  fallback reached by malformed top-level lines such as bare `friend` and
  storage-class-only starts.
