# Parser Whitelist Boundary Audit Todo

Status: Active
Source Idea: ideas/open/07_parser_whitelist_boundary_audit.md
Source Plan: plan.md
Last Updated: 2026-03-29

## Active Item

- Step 3: continue the ranked `NK_EMPTY` audit in
  `src/frontend/parser/declarations.cpp`, starting from the remaining
  top-level parse-and-discard sites around the storage-class / linkage and
  unsupported declaration paths called out in `BOUNDARY_AUDIT.md`

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
- tightened record-member recovery for malformed non-alias `using` members:
  - `src/frontend/parser/types.cpp`
    `try_parse_record_using_member(...)` now parses the non-alias branch as a
    structured qualified-name `using` declaration instead of broadly skipping
    tokens until `;`
  - `src/frontend/parser/types.cpp`
    `is_record_member_recovery_boundary(...)` now lets malformed `using`
    members stop at the next member declaration boundary, not only malformed
    special members
  - added the reduced parse-only regression
    `tests/cpp/internal/parse_only_case/record_member_using_recovery_preserves_following_decl_parse.cpp`
    plus the dedicated dump assertion
    `cpp_parse_record_member_using_recovery_preserves_following_decl_dump` in
    `tests/cpp/internal/InternalTests.cmake`
  - re-ran the required regression guard:
    - baseline: `100% tests passed, 0 tests failed out of 2408`
    - after change: `100% tests passed, 0 tests failed out of 2409`
    - result: monotonic pass-count increase with no newly failing tests
- tightened statement-side local `using` parsing in
  `src/frontend/parser/statements.cpp` so malformed local aliases,
  `using namespace`, and `using` declarations no longer rely on a blind
  skip-until-`;` fallback:
  - valid local `using Alias = type;` aliases now reuse `parse_type_name()`
    and register the parsed alias type instead of a placeholder `int`
  - malformed local `using` forms now recover as `NK_INVALID_STMT` at the next
    local declaration / statement boundary instead of silently erasing the
    following declaration
  - added the reduced parse-only regression
    `tests/cpp/internal/parse_only_case/local_using_alias_recovery_preserves_following_decl_parse.cpp`
    plus the dedicated dump assertion
    `cpp_parse_local_using_alias_recovery_preserves_following_decl_dump` in
    `tests/cpp/internal/InternalTests.cmake`
  - re-ran the required regression guard:
    - baseline: `100% tests passed, 0 tests failed out of 2409`
    - after change: `100% tests passed, 0 tests failed out of 2410`
    - result: monotonic pass-count increase with no newly failing tests

## Next Slice

- compare the newly tightened local `using` boundary behavior against the
  remaining top-level `NK_EMPTY` discard sites in
  `src/frontend/parser/declarations.cpp`, especially the unsupported /
  structure-only declaration exits near the current audit notes
- prefer reduced parse-only repros that show discarded structure or silently
  erased following declarations before changing those `NK_EMPTY` branches
- keep any further record-member recovery widening separate unless a new reduced
  repro shows another non-special-member boundary bug that blocks the
  `NK_EMPTY` audit

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
- the malformed `using Base::` record-member regression is intentionally
  parse-only: the important assertion is that the following `kept` declaration
  remains visible after recovery, not that the malformed `using` line itself is
  semantically accepted
- the malformed local `using Alias = int` regression is intentionally
  parse-only: the important assertion is that the broken local `using` becomes
  `InvalidStmt` while the following `kept` declaration remains visible in the
  AST dump

## Blockers

- none yet
