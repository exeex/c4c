# Parser Disambiguation Matrix Todo

Status: Active
Source Idea: ideas/open/44_parser_disambiguation_matrix_for_type_id_and_declarators.md
Source Plan: plan.md

## Active Item

- Step 2: expand generated `parse_only` coverage beyond the initial
  `grouped_pointer` family using the new declaration-context-aware generator

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

## Next Slice

- extend the generator-backed parse-only slice to adjacent grouped declarators
  such as `grouped_lvalue_ref` and `grouped_rvalue_ref`
- decide whether `member_pointer` should join the next generated batch or stay
  isolated as its own parser-focused follow-up
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
