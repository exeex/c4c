# C++ Parser Surface Normalization Todo

Status: Active
Source Idea: ideas/open/02_parser_cpp_surface_normalization_plan.md
Source Plan: plan.md

## Active Item

- Step 3: Replace `requires` skip paths with dedicated grammar entry points

## Next Slice

- add the narrowest reduced repro where a declaration `requires` clause still
  depends on skip-style parsing instead of a dedicated grammar path
- thread the first explicit `requires` declaration entry point through the
  parser without broadening into concept semantics
- keep follow-up work scoped to parser acceptance and local recovery behavior

## Planned Steps

- [x] Step 1: Normalize lexer keyword coverage
- [x] Step 2: Add explicit parser support for contextual `final` and `override`
- [ ] Step 3: Replace `requires` skip paths with dedicated grammar entry points
- [ ] Step 4: Reconnect to motivating library-facing repros

## Completed

- [x] Activated this runbook from
  `ideas/open/02_parser_cpp_surface_normalization_plan.md`
- [x] Reserved `requires` as a lexer keyword in C++ mode and replaced the
  active parser slice's raw spelling checks with `TokenKind::KwRequires`
- [x] Added `cpp_lex_keyword_requires_tokens` to protect the reduced
  `cpp20_requires_clause_parse.cpp` repro at the lexer boundary
- [x] Rebuilt, ran the focused `requires` tests, and verified the full suite
  remained monotonic with the regression guard
- [x] Added `method_override_final_runtime.cpp` to catch the in-class
  `override` / `final` path where a method body was being dropped
- [x] Consumed `KwOverride` / `KwFinal` in the record-member function suffix
  parser for methods, operator methods, and destructors instead of leaving
  those tokens to recovery
- [x] Rebuilt, ran focused `override` / `final` coverage, and passed the full
  regression guard with `2372/2372` tests passing and zero new failures

## Blockers

- none recorded

## Resume Notes

- keep lambda parsing out of scope unless lifecycle priority changes
- treat `final` and `requires` as parser-acceptance first; defer deeper misuse
  diagnostics
- add reduced tests before parser edits and keep each patch tied to one syntax
  surface
- Step 2 is complete; the next slice should replace the first declaration-side
  `requires` skip path with a dedicated parser entry point
