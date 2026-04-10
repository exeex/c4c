# Active Todo

Status: Active
Source Idea: ideas/open/52_tentative_parse_guard_tiering_and_snapshot_reduction.md
Source Plan: plan.md

## Current Active Item

- Step 3: Migrate the safest common-path probes to the lite guard, starting
  with `sizeof(type)` vs `sizeof(expr)`.

## Pending Items

- Step 3: migrate the safest common-path probes to the lite guard
- Step 4: validate focused parser ambiguities and broader parser regressions
- Step 5: re-measure EASTL parser timing and record follow-on recommendation

## Completed Items

- Activated the source idea into `plan.md` and initialized this execution
  tracker.
- Step 1 inventory completed:
  - `ParserSnapshot` currently records `pos_`, `last_resolved_typedef_`,
    `template_arg_expr_depth_`, `token_mutation_count`, and, under
    `ENABLE_HEAVY_TENTATIVE_SNAPSHOT`, full copies of `typedefs_`,
    `user_typedefs_`, `typedef_types_`, and `var_types_`.
  - `restore_state()` rewinds cursor/bookkeeping and replays token mutations;
    heavy semantic containers are only restored when the heavy snapshot build
    option is enabled.
  - Likely first-wave lite candidates are syntax-only probes in
    `parser_expressions.cpp` (`sizeof(type)` vs `sizeof(expr)`) and
    `parser_statements.cpp` (`if` declaration-condition probing and C++
    range-for detection).
  - Current heavy-by-default sites include template argument type-vs-value
    parsing in `parser_types_declarator.cpp`, qualified-type/declarator probes,
    struct/member ambiguity handling, and declaration probes that can mutate
    typedef/type visibility or `var_types_`.
  - Manual token save/restore remains required at the parenthesized-cast site
    in `parser_expressions.cpp` because it snapshots the full `tokens_` vector
    rather than only the token-mutation journal.
- Step 2 scaffolding completed:
  - added `ParserLiteSnapshot`, `TentativeParseGuardLite`,
    `save_lite_state()`, and `restore_lite_state()` alongside the existing
    heavy guard path
  - kept all call sites on the heavy guard for this slice so parser behavior
    stays unchanged while the tiered API lands
  - tentative parser-debug detail now records `mode=heavy|lite` and per-mode
    lifecycle counts, allowing future migrations to prove heavy-vs-lite usage
  - updated internal parser-debug tests to lock the new heavy-mode detail
  - validation for this slice:
    - targeted `ctest`:
      `cpp_parser_debug_tentative_template_arg_lifecycle`
      `cpp_parser_debug_tentative_cli_only`
    - full suite:
      `3307/3307` passed
    - regression guard:
      `test_fail_before.log` vs `test_fail_after.log` passed with no new
      failures and no pass-count drop

## Next Intended Slice

- switch `parser_expressions.cpp` `sizeof(type)` vs `sizeof(expr)` probing to
  `TentativeParseGuardLite`
- add or confirm focused coverage for the `sizeof` ambiguity path before moving
  additional statement/declaration probes
- only after `sizeof` is stable, evaluate `if` declaration-condition probing
  and range-for detection for the next lite migration wave

## Blockers

- none recorded

## Resume Notes

- keep heavy rollback as the default safe path until a call site is explicitly
  proven lite-safe
- heavy semantic probes in template/declarator/struct parsing remain out of
  scope for the next slice unless `sizeof` uncovers a hidden dependency
- use EASTL `--parse-only` timing as the main performance comparison point
