# Parser Disambiguation Matrix Todo

Status: Active
Source Idea: ideas/open/44_parser_disambiguation_matrix_for_type_id_and_declarators.md
Source Plan: plan.md

## Active Item

- Step 4: reassess whether the next stronger-validation promotion should target
  non-dependent reference-qualified function shapes or isolate the
  dependent-owner `ctx_c_style_cast_target` frontend failure shared by
  function-shaped promotions

## Completed Items

- Activated `ideas/open/44_parser_disambiguation_matrix_for_type_id_and_declarators.md`
  into `plan.md`
- Step 1 audit: confirmed the first concrete gap was not a parser root cause
  yet, but missing matrix assets plus a generator model that only rendered
  abstract type-ids and therefore could not emit valid declaration-context
  code for grouped/function/member-pointer families
- Materialized the tracked manifest assets:
  `ideas/open/44_parser_disambiguation_matrix_patterns.txt`,
  `_by_owner.txt`, and `_by_declarator.txt`
- Landed the first generated `parse_only` family under
  `tests/cpp/internal/generated/parser_disambiguation_matrix/parse_only/`
  for `declarator=grouped_pointer` across all owner spellings and all five
  plan contexts
- Wired generated parse-only matrix cases into
  `tests/cpp/internal/InternalTests.cmake`
- Targeted validation: `ctest --test-dir build -R grouped_pointer
  --output-on-failure` passed all 25 generated cases
- Expanded the generated `parse_only` matrix batch to include
  `grouped_lvalue_ref` and `grouped_rvalue_ref` while preserving the existing
  `grouped_pointer` family in the emitted directory
- Targeted validation: `ctest --test-dir build -R 'grouped_(pointer|lvalue_ref|rvalue_ref)'
  --output-on-failure` passed all 75 generated grouped declarator cases
- Evaluated `member_pointer` as part of Step 2 and kept it in the generated
  `parse_only` batch rather than isolating it for parser-only follow-up
- Added the generated `member_pointer` `parse_only` family under
  `tests/cpp/internal/generated/parser_disambiguation_matrix/parse_only/`
  across all tracked owner spellings and plan contexts
- Targeted validation: `ctest --test-dir build -R 'grouped_(pointer|lvalue_ref|rvalue_ref)|member_pointer'
  --output-on-failure` passed all 100 generated matrix cases plus adjacent
  existing member-pointer coverage with zero failures
- Expanded the generated `parse_only` matrix batch to include
  `function_pointer` while preserving the existing grouped and member-pointer
  families in the emitted directory
- Added the generated `function_pointer` `parse_only` family under
  `tests/cpp/internal/generated/parser_disambiguation_matrix/parse_only/`
  across all tracked owner spellings and plan contexts
- Targeted validation: `ctest --test-dir build -R 'grouped_(pointer|lvalue_ref|rvalue_ref)|member_pointer|function_pointer'
  --output-on-failure` passed all 125 generated matrix cases plus adjacent
  existing function/member-pointer coverage with zero failures
- Full-suite validation: `ctest --test-dir build -j --output-on-failure >
  test_fail_after.log` completed with 3002/3002 passing tests, and
  `check_monotonic_regression.py --before test_fail_before.log --after
  test_fail_after.log --allow-non-decreasing-passed` reported PASS with zero
  new failing tests
- Expanded the generated `parse_only` matrix batch to include
  `function_lvalue_ref` while preserving the existing grouped, member-pointer,
  and `function_pointer` families in the emitted directory
- Added the generated `function_lvalue_ref` `parse_only` family under
  `tests/cpp/internal/generated/parser_disambiguation_matrix/parse_only/`
  across all tracked owner spellings and plan contexts
- Targeted validation: `ctest --test-dir build -R 'function_(pointer|lvalue_ref)'
  --output-on-failure` passed the new 25 generated `function_lvalue_ref`
  cases plus adjacent existing function-pointer coverage with zero failures
- Full-suite validation: `ctest --test-dir build -j --output-on-failure >
  test_fail_after.log` completed with 3027/3027 passing tests, and
  `check_monotonic_regression.py --before test_fail_before.log --after
  test_fail_after.log --allow-non-decreasing-passed` reported PASS with zero
  new failing tests and a +25 pass delta
- Expanded the generated `parse_only` matrix batch to include
  `function_rvalue_ref` while preserving the existing grouped,
  member-pointer, `function_pointer`, and `function_lvalue_ref` families in
  the emitted directory
- Added the generated `function_rvalue_ref` `parse_only` family under
  `tests/cpp/internal/generated/parser_disambiguation_matrix/parse_only/`
  across all tracked owner spellings and plan contexts
- Targeted validation: `ctest --test-dir build -R
  'function_(pointer|lvalue_ref|rvalue_ref)' --output-on-failure` passed all
  75 generated function-shaped matrix cases plus adjacent existing
  function/member-function-pointer coverage with zero failures
- Full-suite validation: `ctest --test-dir build -j --output-on-failure >
  test_fail_after.log` completed with 3052/3052 passing tests, and
  `check_monotonic_regression.py --before test_fail_before.log --after
  test_fail_after.log --allow-non-decreasing-passed` reported PASS with zero
  new failing tests and a +50 pass delta against the stored baseline log
- Expanded the generated `parse_only` matrix batch to include
  `member_function_pointer` while preserving the existing grouped,
  member-pointer, and other function-shaped families in the emitted
  directory
- Added the generated `member_function_pointer` `parse_only` family under
  `tests/cpp/internal/generated/parser_disambiguation_matrix/parse_only/`
  across all tracked owner spellings and plan contexts
- Step 3 parser fix: taught qualified type-head disambiguation to keep
  `qualified-type (owner::*)...` on the type/declarator path in
  statement, cast/type-id, and parameter contexts so the new qualified
  `member_function_pointer` family parses cleanly
- Targeted validation: `ctest --test-dir build -R
  'decl_member_function_pointer|qualified_member_function_pointer_template_owner_parse'
  --output-on-failure` passed all 25 generated `member_function_pointer`
  cases plus adjacent existing qualified member-function-pointer
  coverage with zero failures
- Targeted validation: `ctest --test-dir build -R
  'decl_(function_pointer|function_lvalue_ref|function_rvalue_ref|member_function_pointer)'
  --output-on-failure` passed all 100 generated function-shaped matrix
  cases with zero failures
- Full-suite validation: `ctest --test-dir build -j8 --output-on-failure >
  test_fail_after.log` completed with 3077/3077 passing tests, and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py
  --before test_fail_before.log --after test_fail_after.log
  --allow-non-decreasing-passed` reported PASS with zero new failing
  tests and a +75 pass delta against the stored baseline log
- Reassessed the current function-shaped `parse_only` breadth pass as complete
  enough to begin a bounded Step 4 stronger-validation promotion instead of
  extending Step 2 breadth further
- Added a generated `compile_positive` tier under
  `tests/cpp/internal/generated/parser_disambiguation_matrix/compile_positive/`
  and wired it into `tests/cpp/internal/InternalTests.cmake` as frontend-level
  validation rather than `parse_only`
- Promoted the non-dependent `member_function_pointer` family
  (`simple`, `qualified`, `global_qualified` owners across all five contexts)
  into generated `compile_positive` coverage while keeping the files
  template-driven
- Updated `scripts/generate_parser_disambiguation_matrix.py` so generated test
  headers record tier-appropriate `RUN:` lines and `expected_stage` metadata
- Targeted validation: `ctest --test-dir build -R
  'generated_parser_disambiguation_matrix_(parse_only|compile_positive)_owner_(simple|qualified|global_qualified)__decl_member_function_pointer'
  --output-on-failure` passed all 30 adjacent parse-only plus promoted
  compile-positive cases with zero failures
- Full-suite validation: `ctest --test-dir build -j8 --output-on-failure >
  test_fail_after.log` completed with 3092/3092 passing tests, and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py
  --before test_fail_before.log --after test_fail_after.log
  --allow-non-decreasing-passed` reported PASS with zero new failing
  tests and a +90 pass delta against the stored baseline log
- Promoted the non-dependent `function_pointer` family (`simple`, `qualified`,
  `global_qualified` owners across all five contexts) into generated
  `compile_positive` coverage while preserving the existing promoted
  `member_function_pointer` files in the same generator-owned tier
- Targeted validation: `ctest --test-dir build -R
  'generated_parser_disambiguation_matrix_compile_positive_owner_(simple|qualified|global_qualified)__decl_function_pointer|generated_parser_disambiguation_matrix_compile_positive_owner_(simple|qualified|global_qualified)__decl_member_function_pointer'
  --output-on-failure` passed all 30 promoted `compile_positive`
  function-shaped cases with zero failures
- Full-suite validation: `ctest --test-dir build -j8 --output-on-failure >
  test_fail_after.log` completed with 3107/3107 passing tests, and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py
  --before test_fail_before.log --after test_fail_after.log
  --allow-non-decreasing-passed` reported PASS with zero new failing tests
  and a +105 pass delta against the stored baseline log

## Next Slice

- choose whether the next Step 4 slice should promote non-dependent
  `function_lvalue_ref` / `function_rvalue_ref` cases into
  `compile_positive`, or instead isolate the dependent-owner
  `ctx_c_style_cast_target` frontend failure shared by promoted
  function-shaped families
- if dependent owners become the target, isolate the frontend failure on
  `ctx_c_style_cast_target` where the generated `H<T>::Type` /
  `Rebind<U>::Type` owners currently fail with unknown type-name diagnostics
  in the promoted tier
- keep generation template-driven and separate from the existing hand-written
  reductions in `postive_case/`

## Blockers

- none recorded

## Resume Notes

- activation followed numeric ordering from `ideas/open/`
- keep this initiative test-surface first; do not jump into unrelated parser
  cleanup without tying it to a matrix family
- full-suite regression logs were captured as `test_before.log` and
  `test_after.log`; the detached-HEAD baseline worktree could not materialize
  the external c-testsuite submodule commit, so `test_before.log` reflects the
  clean HEAD suite without those external cases, while the current tree still
  finished with zero failing tests
- the new `compile_positive` promotion is intentionally bounded to
  non-dependent owners; dependent-owner `member_function_pointer`
  `ctx_c_style_cast_target` cases still fail beyond parse-only with unknown
  type-name diagnostics and should be treated as the next focused investigation
  rather than folded silently into this landed slice
- the same unknown type-name frontend failure also reproduces for dependent
  `function_pointer` `ctx_c_style_cast_target` cases under `build/c4cll`, so
  this iteration stays bounded to the three non-dependent owners
