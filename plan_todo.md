# C++ Parser Surface Normalization Todo

Status: Active
Source Idea: ideas/open/02_parser_cpp_surface_normalization_plan.md
Source Plan: plan.md

## Active Item

- Step 2: Add explicit parser support for contextual `final` and `override`

## Next Slice

- add a reduced `override` repro that currently relies on compatibility paths
- thread `KwOverride` through the real declaration sites instead of skip-style
  handling
- keep `final` acceptance aligned with the existing record/class specifier path
  while narrowing where it is consumed

## Planned Steps

- [x] Step 1: Normalize lexer keyword coverage
- [ ] Step 2: Add explicit parser support for contextual `final` and `override`
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

## Blockers

- none recorded

## Resume Notes

- keep lambda parsing out of scope unless lifecycle priority changes
- treat `final` and `requires` as parser-acceptance first; defer deeper misuse
  diagnostics
- add reduced tests before parser edits and keep each patch tied to one syntax
  surface
- `requires` now has lexer-backed classification; next slice should justify
  where `KwOverride` is consumed at the declaration grammar boundary
