# C++ Parser Surface Normalization Todo

Status: Active
Source Idea: ideas/open/02_parser_cpp_surface_normalization_plan.md
Source Plan: plan.md

## Active Item

- Step 3: Replace `requires` skip paths with dedicated grammar entry points
  by handling trailing function-declarator `requires` clauses explicitly

## Next Slice

- extend explicit trailing `requires` handling to the next declaration site
  that still routes through weak recovery or variable-style fallback
- add a reduced repro that proves the next constrained declaration keeps its
  body or following declaration attached
- keep the Step 3 follow-up scoped to parser acceptance and local recovery

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
- [x] Replaced the top-level template `requires` token-drain helper with a
  dedicated clause parser that consumes a constraint-expression via `parse_expr`
- [x] Added `cpp20_requires_clause_preserves_following_decl.cpp` to prove a
  constrained template no longer swallows a following top-level declaration
- [x] Rebuilt, ran focused `requires` coverage, and passed the full regression
  guard with `2373/2373` tests passing and zero new failures
- [x] Added `cpp20_trailing_requires_clause_parse.cpp` to prove a constrained
  function keeps its trailing `requires` clause and body attached
- [x] Routed top-level trailing function `requires` clauses through an
  explicit parser helper that stops before the real function body while still
  accepting nested `requires { ... }` blocks
- [x] Rebuilt, ran focused `requires` coverage, and passed the full regression
  guard with `2374/2374` tests passing and zero new failures

## Blockers

- none recorded

## Resume Notes

- keep lambda parsing out of scope unless lifecycle priority changes
- treat `final` and `requires` as parser-acceptance first; defer deeper misuse
  diagnostics
- add reduced tests before parser edits and keep each patch tied to one syntax
  surface
- the first declaration-side `requires` clause path now uses an explicit parser
  entry point; continue Step 3 by targeting the next remaining skip-style site
- top-level constrained function definitions now parse through their trailing
  `requires` clause without dropping the function body into expression parsing
