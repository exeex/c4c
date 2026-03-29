# C++ Parser Surface Normalization Todo

Status: Active
Source Idea: ideas/open/02_parser_cpp_surface_normalization_plan.md
Source Plan: plan.md

## Active Item

- Step 4: Reconnect to motivating library-facing repros by rerunning the
  parser-facing `std::vector` or libstdc++ slices that depend on normalized
  `requires` parsing

## Next Slice

- rerun the motivating reduced libstdc++ or `std::vector` repro that first
  exposed the constrained declaration parsing gaps
- record any surviving blocker as a separate follow-up idea instead of
  broadening this runbook back into general library bring-up
- keep the next slice scoped to confirming the parser frontier moved

## Planned Steps

- [x] Step 1: Normalize lexer keyword coverage
- [x] Step 2: Add explicit parser support for contextual `final` and `override`
- [x] Step 3: Replace `requires` skip paths with dedicated grammar entry points
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
- [x] Added `cpp20_record_member_trailing_requires_frontend.cpp` to catch
  constrained record-member constructors, conversion operators, and ordinary
  methods that were dropping their bodies during member parsing
- [x] Routed record-member constructor, conversion-operator, and ordinary
  method paths through explicit trailing `requires` handling after
  exception-spec and trailing-return parsing
- [x] Rebuilt, ran focused `requires` and record-member coverage, and passed
  the full regression guard with `2375/2375` tests passing and zero new
  failures
- [x] Added `cpp20_out_of_class_trailing_requires_runtime.cpp` to catch
  constrained out-of-class constructor and conversion-operator definitions
  whose trailing `requires` clauses were bypassing the specialized parser path
- [x] Taught the out-of-class special-member recognizer to stop at
  template-qualified owners, consume trailing `requires` clauses before
  constructor initializer lists or bodies, and avoid speculative token-stream
  mutation during owner probing
- [x] Rebuilt, ran the focused constrained-member coverage plus the exposed
  parser-debug/template regressions, and passed the full regression guard with
  `2376/2376` tests passing and zero new failures

## Blockers

- none recorded

## Resume Notes

- keep lambda parsing out of scope unless lifecycle priority changes
- treat `final` and `requires` as parser-acceptance first; defer deeper misuse
  diagnostics
- add reduced tests before parser edits and keep each patch tied to one syntax
  surface
- top-level constrained function definitions now parse through their trailing
  `requires` clause without dropping the function body into expression parsing
- constrained record members now keep constructor, conversion-operator, and
  ordinary method bodies attached after instantiation
- constrained out-of-class constructors and conversion operators now parse
  through explicit trailing `requires` handling even when the owner is written
  as a template-id such as `holder<T>::...`
