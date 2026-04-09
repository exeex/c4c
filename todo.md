# Parser Disambiguation Matrix Todo

Status: Active
Source Idea: ideas/open/44_parser_disambiguation_matrix_for_type_id_and_declarators.md
Source Plan: plan.md

## Active Item

- Step 5: close out the function-shaped `parse_only` breadth pass with
  the validated `member_function_pointer` family recorded, then reassess
  whether Step 2 is complete enough to move into selected stronger
  validation promotion work

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

## Next Slice

- reassess whether Step 2 breadth is now complete for the currently
  planned function-shaped declarator set
- if no more missing parse-only families remain, move to Step 4 and
  choose a bounded high-risk family for stronger-than-parse-only
  validation
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
