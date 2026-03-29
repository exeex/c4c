# Parser Whitelist Boundary Audit Todo

Status: Active
Source Idea: ideas/open/07_parser_whitelist_boundary_audit.md
Source Plan: plan.md
Last Updated: 2026-03-29

## Active Item

- Step 3: decide whether the remaining record-member recovery gap should be
  widened beyond malformed special-member signatures or whether the next
  highest-value boundary is now one of the remaining `NK_EMPTY` discard sites

## Completed

- switched the active runbook away from the `std::vector` bring-up and folded
  the latest parser-hygiene findings back into
  `ideas/open/04_std_vector_bringup_plan.md`
- activated `ideas/open/07_parser_whitelist_boundary_audit.md` as the new
  active plan
- captured the most recent reduced evidence motivating the switch:
  - `iterator_concepts.h` plus `functional_hash.h` had reduced to a
    `requires`-clause boundary issue
  - tightening `skip_cpp20_constraint_atom(...)` improved the reduced repro
  - added
    `tests/cpp/internal/postive_case/cpp20_requires_trait_disjunction_function_parse.cpp`
    to hold that narrower case in-tree
- completed the Step 1 parser-boundary inventory in
  `src/frontend/parser/BOUNDARY_AUDIT.md`, covering:
  - duplicated `requires` clause boundary helpers in `declarations.cpp` and
    `types.cpp`
  - generic record-member recovery
  - broad `is_type_start()` / `can_start_parameter_type()` probes
  - representative `NK_EMPTY` parse-and-discard sites
- ranked the first tightening targets from that inventory:
  - highest risk: the older `types.cpp` `requires` boundary helpers still using
    `is_type_start()` as a declaration-boundary fallback
  - next: trailing `requires` clause termination in `declarations.cpp`
  - next: record-member recovery that can still advance to `}`
- aligned the duplicated `src/frontend/parser/types.cpp`
  `is_cpp20_requires_clause_decl_boundary(...)` helper with the narrower
  declaration-side whitelist so it no longer treats `Identifier` after `::` or
  before `<` as an automatic declaration boundary
- added
  `tests/cpp/internal/postive_case/cpp20_requires_trait_disjunction_member_parse.cpp`
  and registered it in `tests/cpp/internal/InternalTests.cmake` to keep the
  record-member template `requires` path covered
- re-ran the required regression guard:
  - baseline: `100% tests passed, 0 tests failed out of 2404`
  - after change: `100% tests passed, 0 tests failed out of 2405`
  - result: monotonic pass-count increase from the new targeted test, with no
    newly failing cases
- tightened the duplicated trailing `requires` clause helpers in
  `src/frontend/parser/declarations.cpp` and `src/frontend/parser/types.cpp`
  so they now reuse the existing atom-by-atom constraint walk instead of a
  broad consume-until-stop loop
- added the reduced parse regression
  `tests/cpp/internal/postive_case/cpp20_trailing_requires_following_member_decl_parse.cpp`
  plus a dedicated dump assertion in
  `tests/cpp/internal/InternalTests.cmake` to prove a malformed constrained
  member no longer swallows the following `inner` declaration
- narrowed `src/frontend/parser/types.cpp`
  `recover_record_member_parse_error(int)` so malformed special-member
  signatures can stop at the next simple field declaration instead of silently
  erasing it through broad recovery
- added the parse-only regression
  `tests/cpp/internal/parse_only_case/record_member_recovery_preserves_following_decl_parse.cpp`
  plus the dedicated dump assertion
  `cpp_parse_record_member_recovery_preserves_following_decl_dump` in
  `tests/cpp/internal/InternalTests.cmake`
- re-ran the required regression guard:
  - baseline: `100% tests passed, 0 tests failed out of 2391`
  - after change: `100% tests passed, 0 tests failed out of 2408`
  - result: monotonic pass-count increase with no newly failing tests

## Next Slice

- decide whether `recover_record_member_parse_error(int)` should grow beyond
  the new special-member sync path or whether the next Step 3 batch should move
  to the ranked `NK_EMPTY` parse-and-discard sites in
  `src/frontend/parser/BOUNDARY_AUDIT.md`
- if record-member recovery remains the priority, add the next reduced repro
  for a non-special-member malformed declaration before widening the recovery
  boundary again
- if the next best risk is an `NK_EMPTY` discard path instead, switch the
  active item explicitly in `plan_todo.md` rather than silently broadening this
  slice

## Notes

- this switch is intentional: recent debugging showed that continuing `<vector>`
  reduction on top of broad parser rules was becoming lower-leverage than
  tightening the parser boundaries themselves
- the parked `std::vector` idea should be revisited after the first whitelist
  audit fixes land and the remaining suspicious sites are ranked
- this iteration is intentionally scoped to the older `types.cpp` duplicate
  helpers first and then the duplicated trailing `requires` termination path
- the new member-template regression currently parses before and after the
  helper alignment, so its value is as a guard against future fallback
  broadening on the `types.cpp` path rather than as a newly red-to-green test
- the trailing `requires` follow-set regression is intentionally parse-only:
  the important assertion is that the following member declaration remains
  visible in the AST dump, not that this malformed source compiles cleanly
- the Step 1 inventory now lives in `src/frontend/parser/BOUNDARY_AUDIT.md` so
  future parser work can reuse the current shortlist instead of rebuilding it
  from scratch
- the new record-member recovery regression is intentionally stored under
  `tests/cpp/internal/parse_only_case/` so it does not get auto-promoted into
  the positive or negative compile-test globs

## Blockers

- none yet
