# Parser Disambiguation Matrix Todo

Status: Active
Source Idea: ideas/open/44_parser_disambiguation_matrix_for_type_id_and_declarators.md
Source Plan: plan.md

## Active Item

- Step 5: audit the generated parser-disambiguation matrix manifests and
  emitted test tiers after closing the dependent-owner function-shaped
  `compile_positive` promotions, then record any remaining deferred promotion
  gaps as coverage notes instead of forcing broader frontend work

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
- Promoted the non-dependent `function_lvalue_ref` and
  `function_rvalue_ref` families (`simple`, `qualified`, `global_qualified`
  owners across all five contexts) into generated `compile_positive`
  coverage while preserving the existing promoted `function_pointer` and
  `member_function_pointer` files in the same generator-owned tier
- Generator fix: taught `scripts/generate_parser_disambiguation_matrix.py`
  to emit initialized local-declaration forms for `compile_positive`
  function-reference cases by binding them to `support_return<owner>`
  helpers, keeping the promoted reference-shaped files frontend-valid
- Targeted validation: `ctest --test-dir build -R
  'generated_parser_disambiguation_matrix_compile_positive_owner_(simple|qualified|global_qualified)__decl_function_(lvalue_ref|rvalue_ref|pointer)|generated_parser_disambiguation_matrix_compile_positive_owner_(simple|qualified|global_qualified)__decl_member_function_pointer'
  --output-on-failure` passed all 60 adjacent promoted function-shaped
  `compile_positive` cases with zero failures
- Full-suite validation: `ctest --test-dir build -j8 --output-on-failure >
  test_fail_after.log` completed with 3137/3137 passing tests, and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py
  --before test_fail_before.log --after test_fail_after.log
  --allow-non-decreasing-passed` reported PASS with zero new failing tests
  and a +30 pass delta against the refreshed pre-slice baseline log
- Promoted the dependent-owner `ctx_c_style_cast_target`
  `function_pointer`, `function_lvalue_ref`, and `function_rvalue_ref`
  families (`dependent_typename` and `dependent_template_member`) into
  generated `compile_positive` coverage with six new generated cases under
  `tests/cpp/internal/generated/parser_disambiguation_matrix/compile_positive/`
- Step 4 frontend fix: taught `src/frontend/sema/validate.cpp` to defer
  decorated unresolved template-parameter placeholder names in cast targets,
  which unblocks dependent owner function-shaped promotions without changing
  ordinary bare unknown-type diagnostics
- Targeted validation: `ctest --test-dir build -R
  'generated_parser_disambiguation_matrix_compile_positive_owner_(dependent_typename|dependent_template_member)__decl_function_(pointer|lvalue_ref|rvalue_ref)__ctx_c_style_cast_target'
  --output-on-failure` passed all 6 newly promoted dependent-owner cases
- Targeted validation: `ctest --test-dir build -R
  'generated_parser_disambiguation_matrix_compile_positive_owner_(simple|qualified|global_qualified|dependent_typename|dependent_template_member)__decl_function_(pointer|lvalue_ref|rvalue_ref)|generated_parser_disambiguation_matrix_compile_positive_owner_(simple|qualified|global_qualified)__decl_member_function_pointer'
  --output-on-failure` passed all 66 adjacent promoted function-shaped
  `compile_positive` cases with zero failures
- Targeted validation: `ctest --test-dir build -R
  'cpp_positive_sema_c_style_cast_(dependent_typename|dependent_template_member)|qualified_member_function_pointer_template_owner_parse'
  --output-on-failure` passed 5 adjacent dependent cast/member-function-pointer
  regressions with zero failures
- Full-suite validation: `ctest --test-dir build -j8 --output-on-failure >
  test_fail_after.log` completed with 3153/3153 passing tests, and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py
  --before test_fail_before.log --after test_fail_after.log
  --allow-non-decreasing-passed` reported PASS with zero new failing tests
  and a +8 pass delta against the refreshed pre-slice baseline log
- Full-suite validation: `ctest --test-dir build -j8 --output-on-failure >
  test_fail_after.log` completed with 3143/3143 passing tests, and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py
  --before test_fail_before.log --after test_fail_after.log
  --allow-non-decreasing-passed` reported PASS with zero new failing tests
  and a +36 pass delta against the stored baseline log
- Promoted the dependent-owner `member_function_pointer`
  `ctx_c_style_cast_target` family (`dependent_typename` and
  `dependent_template_member`) into generated `compile_positive` coverage with
  two new generated cases under
  `tests/cpp/internal/generated/parser_disambiguation_matrix/compile_positive/`
- Probe result: the same cast-validation fix already covered this family, so
  the promotion required no additional parser or sema changes beyond adding the
  generator-shaped promoted files
- Targeted validation: `ctest --test-dir build -R
  'generated_parser_disambiguation_matrix_compile_positive_owner_(dependent_typename|dependent_template_member)__decl_member_function_pointer__ctx_c_style_cast_target'
  --output-on-failure` passed both newly promoted dependent-owner
  `member_function_pointer` cases
- Targeted validation: `ctest --test-dir build -R
  'generated_parser_disambiguation_matrix_compile_positive_owner_(simple|qualified|global_qualified|dependent_typename|dependent_template_member)__decl_(function_pointer|function_lvalue_ref|function_rvalue_ref|member_function_pointer)__ctx_c_style_cast_target'
  --output-on-failure` passed all 20 adjacent promoted cast-target
  function-shaped cases with zero failures
- Targeted validation: `ctest --test-dir build -R
  'cpp_positive_sema_c_style_cast_(dependent_typename|dependent_template_member)|qualified_member_function_pointer_template_owner_parse'
  --output-on-failure` passed 5 adjacent dependent cast/member-function-pointer
  regressions with zero failures
- Full-suite validation: `ctest --test-dir build -j8 --output-on-failure >
  test_fail_after.log` completed with 3145/3145 passing tests, and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py
  --before test_fail_before.log --after test_fail_after.log
  --allow-non-decreasing-passed` reported PASS with zero new failing tests
  and a +38 pass delta against the stored baseline log
- Promoted the remaining dependent-owner `member_function_pointer`
  `compile_positive` contexts (`local_declaration`, `parameter_declaration`,
  `parenthesized_type_id_consumer`, and `ambiguous_statement_context`) for both
  `dependent_typename` and `dependent_template_member`, adding eight generated
  files under
  `tests/cpp/internal/generated/parser_disambiguation_matrix/compile_positive/`
- Probe result: the generator already emitted frontend-valid source for these
  remaining dependent-owner `member_function_pointer` contexts, so the slice
  landed as pure stronger-validation coverage with no parser or sema changes
- Targeted validation: `ctest --test-dir build -R
  'generated_parser_disambiguation_matrix_compile_positive_owner_(dependent_typename|dependent_template_member)__decl_member_function_pointer__(ctx_(local_declaration|parameter_declaration|parenthesized_type_id_consumer|ambiguous_statement_context)|ctx_c_style_cast_target)'
  --output-on-failure` passed all 10 dependent-owner promoted
  `member_function_pointer` cases
- Targeted validation: `ctest --test-dir build -R
  'generated_parser_disambiguation_matrix_compile_positive_owner_(simple|qualified|global_qualified|dependent_typename|dependent_template_member)__decl_(function_pointer|function_lvalue_ref|function_rvalue_ref|member_function_pointer)'
  --output-on-failure` passed all 76 adjacent promoted function-shaped
  `compile_positive` cases with zero failures
- Targeted validation: `ctest --test-dir build -R
  'cpp_positive_sema_c_style_cast_(dependent_typename|dependent_template_member)|qualified_member_function_pointer_template_owner_parse'
  --output-on-failure` passed 5 adjacent dependent cast/member-function-pointer
  regressions with zero failures

## Next Slice

- audit the manifest and generated directory so the recorded coverage matches
  the emitted `parse_only` and `compile_positive` tiers after the final
  dependent-owner `member_function_pointer` promotion
- decide whether Step 5 should close with coverage notes only or also record a
  deferred follow-up idea for any future `runtime_positive` promotion work
- keep the next slice focused on auditable coverage/closure notes rather than
  widening the current runbook into new frontend feature work

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
- the dependent function-shaped `ctx_c_style_cast_target` promotion now lands
  cleanly for both `typename H<T>::Type` and
  `typename H<T>::template Rebind<U>::Type`; the next adjacent check is the
  generated coverage audit now that dependent-owner
  `member_function_pointer` `compile_positive` promotion is complete across all
  five plan contexts
- the non-dependent function-reference promotion uncovered a generator-shape
  issue rather than a parser defect: `compile_positive` local declarations for
  function references must be initialized, so the generator now binds those
  cases to `support_return<owner>` helpers
