# Parser Whitelist Boundary Audit Todo

Status: Active
Source Idea: ideas/open/07_parser_whitelist_boundary_audit.md
Source Plan: plan.md
Last Updated: 2026-03-29

## Active Item

- Step 3: continue through the remaining top-level structure-only /
  unsupported `NK_EMPTY` exits in `src/frontend/parser/declarations.cpp` after
  tightening the empty namespace-wrapper path after the empty `extern "C"`
  linkage-block wrapper slice

## Completed

- tightened the generic top-level no-type-start recovery in
  `src/frontend/parser/declarations.cpp` so malformed discarded declarations
  now stop at the shared declaration-boundary helper instead of swallowing a
  later `class` definition:
  - expanded `is_top_level_decl_recovery_boundary(...)` to recognize
    `class`, `namespace`, `constexpr`, and `consteval`
  - replaced the duplicated hard-coded fallback stop list with the shared
    helper so future boundary updates stay aligned
- added the reduced parse-only regression
  `tests/cpp/internal/parse_only_case/top_level_class_recovery_preserves_following_decl_parse.cpp`
  plus the dedicated dump assertion
  `cpp_parse_top_level_class_recovery_preserves_following_decl_dump` in
  `tests/cpp/internal/InternalTests.cmake`
- updated `src/frontend/parser/BOUNDARY_AUDIT.md` to record the generic
  top-level no-type-start recovery as a covered boundary with reduced `class`
  evidence
- re-ran the required regression guard:
  - baseline: `99% tests passed, 1 test failed out of 2414`
  - after change: `99% tests passed, 1 test failed out of 2415`
  - result: monotonic pass-count increase from the new targeted test, with no
    newly failing cases
- tightened malformed top-level `using namespace` / `using ns::name` recovery in
  `src/frontend/parser/declarations.cpp` so a missing `;` now stops before the
  next strong declaration starter instead of relying on a blind semicolon match
- added the reduced parse-only regression
  `tests/cpp/internal/parse_only_case/top_level_using_namespace_recovery_preserves_following_decl_parse.cpp`
  plus the dedicated dump assertion
  `cpp_parse_top_level_using_namespace_recovery_preserves_following_decl_dump`
  in `tests/cpp/internal/InternalTests.cmake`
- updated `src/frontend/parser/BOUNDARY_AUDIT.md` to record top-level
  `using namespace` / `using ns::name` as a covered `NK_EMPTY` recovery
  boundary and to note the remaining `using Alias = ...` missing-semicolon case
  as a still-suspicious follow-on site
- re-ran the required regression guard:
  - baseline: `97% tests passed, 54 tests failed out of 2413`
  - after change: `99% tests passed, 1 test failed out of 2413`
  - result: monotonic pass-count increase with no newly failing tests
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
- tightened the malformed top-level storage-class fallback in
  `src/frontend/parser/declarations.cpp` so unsupported forms like
  `extern foo` no longer blindly `skip_until(';')` across the following
  declaration:
  - the recovery now stops at strong declaration starters on a later line,
    which preserves the next top-level declaration while keeping the malformed
    storage-class fragment discarded as `NK_EMPTY`
  - added the reduced parse-only regression
    `tests/cpp/internal/parse_only_case/top_level_storage_class_recovery_preserves_following_decl_parse.cpp`
    plus the dedicated dump assertion
    `cpp_parse_top_level_storage_class_recovery_preserves_following_decl_dump`
    in `tests/cpp/internal/InternalTests.cmake`
  - updated `src/frontend/parser/BOUNDARY_AUDIT.md` to mark this fallback as a
    covered `NK_EMPTY` site with concrete evidence
  - re-ran the required regression guard:
    - baseline: `100% tests passed, 0 tests failed out of 2409`
    - after change: `100% tests passed, 0 tests failed out of 2411`
    - result: monotonic pass-count increase with no newly failing tests
- tightened malformed top-level `asm(...)` recovery in
  `src/frontend/parser/declarations.cpp` so an unterminated `asm(` payload no
  longer swallows the following declaration through a blind paren-group skip:
  - added the reduced parse-only regression
    `tests/cpp/internal/parse_only_case/top_level_asm_recovery_preserves_following_decl_parse.cpp`
    plus the dedicated dump assertion
    `cpp_parse_top_level_asm_recovery_preserves_following_decl_dump`
    in `tests/cpp/internal/InternalTests.cmake`
  - updated `src/frontend/parser/BOUNDARY_AUDIT.md` to record top-level
    `asm(...)` as a covered `NK_EMPTY` recovery boundary
  - re-ran the required regression guard:
    - baseline: `100% tests passed, 0 tests failed out of 2411`
    - after change: `100% tests passed, 0 tests failed out of 2412`
    - result: monotonic pass-count increase with no newly failing tests

## Next Slice

- after the empty `extern "C"` linkage-block slice, continue through the
  remaining top-level `NK_EMPTY` discard sites in
  `src/frontend/parser/declarations.cpp`, especially the still-unreviewed
  structure-only / unsupported declaration exits beyond linkage wrappers
- prefer another reduced parse-only repro that shows discarded structure or a
  silently erased following declaration before changing each remaining
  `NK_EMPTY` branch
- keep any further record-member recovery widening separate unless a new reduced
  repro shows another non-special-member boundary bug that blocks the
  top-level `NK_EMPTY` audit

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
- the malformed top-level `extern foo` regression is intentionally parse-only:
  the important assertion is that the following `kept` global remains visible
  after recovery, not that the broken storage-class fragment is diagnosed as a
  first-class declaration node
- the malformed top-level `asm(` regression is intentionally parse-only: the
  important assertion is that the following `kept` global remains visible after
  recovery, not that the unsupported top-level asm fragment is elevated into a
  first-class declaration node
- tightened malformed top-level `using Alias = ...` recovery in
  `src/frontend/parser/declarations.cpp` so a missing `;` now rewinds from the
  alias type parse and stops before a later-line swallowed declaration instead
  of accepting it as part of the alias type
- added the reduced parse-only regression
  `tests/cpp/internal/parse_only_case/top_level_using_alias_recovery_preserves_following_decl_parse.cpp`
  plus the dedicated dump assertion
  `cpp_parse_top_level_using_alias_recovery_preserves_following_decl_dump` in
  `tests/cpp/internal/InternalTests.cmake`
- updated `src/frontend/parser/BOUNDARY_AUDIT.md` to mark top-level
  `using Alias = ...` missing-semicolon recovery as a covered `NK_EMPTY`
  boundary instead of a still-suspicious follow-on site
- the next top-level `NK_EMPTY` slice is the generic recovery path triggered
  when no type start is recognized; that boundary is now covered by a reduced
  `class kept {}` regression, so the next work should focus on the remaining
  structure-only / unsupported exits that still return `NK_EMPTY`
- tightened empty top-level `extern "C"` linkage blocks in
  `src/frontend/parser/declarations.cpp` so they no longer materialize a
  synthetic `NK_EMPTY` node when the block contains no declarations
- added the reduced parse-only regression
  `tests/cpp/internal/parse_only_case/top_level_extern_c_empty_block_preserves_following_decl_parse.cpp`
  plus the dedicated dump assertion
  `cpp_parse_top_level_extern_c_empty_block_preserves_following_decl_dump` in
  `tests/cpp/internal/InternalTests.cmake`
- updated `src/frontend/parser/BOUNDARY_AUDIT.md` to record empty top-level
  `extern "C"` linkage wrappers as a covered structure-only boundary with
  reduced evidence
- re-ran the required regression guard:
  - baseline: `99% tests passed, 1 test failed out of 2415`
  - after change: `99% tests passed, 1 test failed out of 2416`
  - result: monotonic pass-count increase from the new targeted test, with no
    newly failing cases
- tightened empty top-level `namespace {}` / `namespace ns {}` wrappers in
  `src/frontend/parser/declarations.cpp` so they no longer materialize a
  synthetic `NK_EMPTY` node when the block contains no declarations
- added the reduced parse-only regression
  `tests/cpp/internal/parse_only_case/top_level_empty_namespace_block_preserves_following_decl_parse.cpp`
  plus the dedicated dump assertion
  `cpp_parse_top_level_empty_namespace_block_preserves_following_decl_dump` in
  `tests/cpp/internal/InternalTests.cmake`
- updated `src/frontend/parser/BOUNDARY_AUDIT.md` to record empty top-level
  namespace wrappers as a covered structure-only boundary with reduced
  evidence
- re-ran the required regression guard:
  - baseline: `99% tests passed, 1 test failed out of 2416`
  - after change: `99% tests passed, 1 test failed out of 2417`
  - result: monotonic pass-count increase from the new targeted test, with no
    newly failing cases; the lone remaining failure is still
    `cpp_positive_sema_iterator_concepts_following_hash_base_parse_cpp`

## Blockers

- none yet
