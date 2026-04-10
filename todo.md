# Active Todo

Status: Active
Source Idea: ideas/open/52_tentative_parse_guard_tiering_and_snapshot_reduction.md
Source Plan: plan.md

## Current Active Item

- Step 3: evaluate C++ range-for detection as the next lite-safe
  statement-path probe after the `if (...)` declaration-condition migration.

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
- Step 3 `sizeof` slice completed:
  - switched `parser_expressions.cpp` `sizeof(type)` vs `sizeof(expr)` probing
    to `TentativeParseGuardLite`
  - added `tests/cpp/internal/negative_case/parser_debug_sizeof_type_tentative_lite.cpp`
    plus `cpp_parser_debug_sizeof_type_tentative_lite` to lock the lite-mode
    tentative commit detail on the `sizeof(type)` path
  - targeted validation passed:
    - `cpp_parser_debug_tentative_template_arg_lifecycle`
    - `cpp_parser_debug_tentative_cli_only`
    - `cpp_parser_debug_sizeof_type_tentative_lite`
  - full clean rebuild + suite result:
    - `3307/3309` passed, `2` failed
  - regression guard failed because the fresh `test_fail_after.log` added two
      pre-existing positive-case failures not present in the stale baseline log:
      `cpp_positive_sema_scoped_enum_bitwise_runtime_cpp` and
      `cpp_positive_sema_template_angle_bracket_validation_cpp`
  - confirmed both failures reproduce on a pristine `HEAD` snapshot and emit
      invalid LLVM IR unrelated to `sizeof` tentative parsing, so this slice did
      not introduce them
- Step 3 `if` declaration-condition slice completed:
  - switched `parser_statements.cpp` `if (...)` declaration-condition probing
    to use `TentativeParseGuardLite` for syntax-only heads while keeping
    `struct`/`class`/`union`/`enum`, `typename`, global-qualified, and
    template-id-headed probes on the heavy path
  - added
    `tests/cpp/internal/negative_case/parser_debug_if_condition_decl_tentative_lite.cpp`
    plus `cpp_parser_debug_if_condition_decl_tentative_lite` to lock the
    declaration-condition tentative commit onto `mode=lite`
  - targeted validation passed:
    - `cpp_positive_sema_if_condition_decl_parse_cpp`
    - `cpp_positive_sema_cpp20_if_likely_stmt_attr_parse_cpp`
    - `cpp_positive_sema_pseudo_destructor_if_else_parse_cpp`
    - `cpp_positive_sema_range_for_const_cpp`
    - `cpp_positive_sema_fixed_vec_smoke_cpp`
    - `cpp_parser_debug_tentative_template_arg_lifecycle`
    - `cpp_parser_debug_tentative_cli_only`
    - `cpp_parser_debug_sizeof_type_tentative_lite`
    - `cpp_parser_debug_if_condition_decl_tentative_lite`
  - fresh full-suite regression guard passed:
    - `test_fail_before.log`: `1348/1363` passed, `15` failed
    - `test_fail_after.log`: `3309/3311` passed, `2` failed
    - monotonic check reported `0` newly failing tests and resolved `13`
      pre-existing failures from the stale baseline

## Next Intended Slice

- inventory the C++ range-for detection probe in `parser_statements.cpp` for
  lite-safety, especially around declaration parsing and `:` vs `;`
  disambiguation
- add focused parser-debug or parse coverage before switching the range-for
  tentative guard, and keep heavy rollback for any declaration head that can
  mutate semantic parser state

## Blockers

- none for the active Step 3 path; the remaining two full-suite failures in
  `test_fail_after.log`
  (`cpp_positive_sema_scoped_enum_bitwise_runtime_cpp`,
  `cpp_positive_sema_template_angle_bracket_validation_cpp`) also reproduce on
  pristine `HEAD`

## Resume Notes

- keep heavy rollback as the default safe path until a call site is explicitly
  proven lite-safe
- for the current slice, do not downgrade `if` declaration probes that start
  with `struct`/`class`/`union`/`enum`; those can still mutate semantic parser
  state during base-type parsing
- heavy semantic probes in template/declarator/struct parsing remain out of
  scope for the next slice unless `sizeof` uncovers a hidden dependency
- the `sizeof` lite probe still nests heavy tentative parsing inside
  `parse_type_name()`; only the outer ambiguity guard moved to lite, which is
  the intended incremental boundary for this slice
- the `if` declaration-condition migration is intentionally tiered: simple
  builtin/qualifier/typedef-headed probes can use lite rollback, while
  stateful declaration heads stay on the heavy guard until separately proven
- use EASTL `--parse-only` timing as the main performance comparison point
