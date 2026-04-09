# Parser Disambiguation Matrix Todo

Status: Active
Source Idea: ideas/open/44_parser_disambiguation_matrix_for_type_id_and_declarators.md
Source Plan: plan.md

## Active Item

- Step 2: extend generated `parse_only` coverage into the next
  function-shaped declarator family by emitting `function_rvalue_ref`
  across all tracked owner spellings and all five plan contexts, then
  validate that generated batch before considering
  `member_function_pointer` or a Step 3 parser-fix split

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

## Next Slice

- extend Step 2 breadth into the remaining function-shaped declarator families
  after `function_lvalue_ref`, with `function_rvalue_ref` as the current
  generated `parse_only` target across all owner spellings and plan contexts
- if the function-shaped family reveals a concrete parser split failure,
  isolate that failing family as the Step 3 parser-fix slice
- if `function_rvalue_ref` lands cleanly, continue with generated
  `member_function_pointer` `parse_only` coverage next
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
