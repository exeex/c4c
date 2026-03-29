# Parser Whitelist Boundary Audit Todo

Status: Active
Source Idea: ideas/open/07_parser_whitelist_boundary_audit.md
Source Plan: plan.md
Last Updated: 2026-03-29

## Active Item

- Step 3: tighten the next parser boundary batch by narrowing
  `recover_record_member_parse_error(int)` in `src/frontend/parser/types.cpp`
  so malformed members cannot silently drift to `}` and hide the real failure
  site

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

## Next Slice

- use the updated shortlist in `src/frontend/parser/BOUNDARY_AUDIT.md` and
  confirm the next highest-risk site is still record-member recovery that can
  advance to `}`
- add a reduced repro that proves malformed members stop at a bounded recovery
  point instead of erasing the later member that should surface the error
- if narrowing that recovery uncovers a separate unsupported syntax gap, split
  it into a new `ideas/open/*.md` note rather than silently widening this plan

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

## Blockers

- none yet
