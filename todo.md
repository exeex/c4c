# Parser Disambiguation Matrix Todo

Status: Active
Source Idea: ideas/open/44_parser_disambiguation_matrix_for_type_id_and_declarators.md
Source Plan: plan.md

## Active Item

- Step 2: carry the next function-shaped declarator family into the generated
  `parse_only` matrix, starting with `function_pointer` unless a narrower
  parser-only failure surfaces first

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

## Next Slice

- extend Step 2 breadth into the function-shaped declarator families, starting
  with generated `function_pointer` `parse_only` coverage across all owner
  spellings and plan contexts
- if the function-shaped family reveals a concrete parser split failure,
  isolate that failing family as the Step 3 parser-fix slice
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
