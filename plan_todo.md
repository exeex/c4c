# Parser Whitelist Boundary Audit Todo

Status: Active
Source Idea: ideas/open/07_parser_whitelist_boundary_audit.md
Source Plan: plan.md
Last Updated: 2026-03-29

## Active Item

- Step 3: tighten the first batch by aligning the duplicated `requires`
  boundary helpers in `src/frontend/parser/types.cpp` with the narrower
  declaration-side whitelist, and prove the record-member template path with a
  dedicated reduced parse test

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

## Next Slice

- after the `types.cpp` helper alignment lands, re-run the boundary shortlist
  and confirm the next highest-risk site is still the trailing `requires`
  clause loop in `src/frontend/parser/declarations.cpp`
- if the reduced member-template test exposes any remaining false positives,
  split that into a separate follow-on idea instead of expanding this slice
- if the shortlist stays stable, move the active item to the trailing
  `requires` clause termination loop and add a reduced test that proves it
  stops on an explicit follow-set rather than generic token consumption

## Notes

- this switch is intentional: recent debugging showed that continuing `<vector>`
  reduction on top of broad parser rules was becoming lower-leverage than
  tightening the parser boundaries themselves
- the parked `std::vector` idea should be revisited after the first whitelist
  audit fixes land and the remaining suspicious sites are ranked
- current uncommitted implementation work in the tree still includes:
  - tightened `requires`-clause boundary logic in
    `src/frontend/parser/declarations.cpp`
  - the new reduced regression test for requires-trait disjunction parsing
- this iteration is intentionally scoped to the older `types.cpp` duplicate
  helpers that are used by record-member template preludes
- the new member-template regression currently parses before and after the
  helper alignment, so its value is as a guard against future fallback
  broadening on the `types.cpp` path rather than as a newly red-to-green test
- the Step 1 inventory now lives in `src/frontend/parser/BOUNDARY_AUDIT.md` so
  future parser work can reuse the current shortlist instead of rebuilding it
  from scratch

## Blockers

- none yet
