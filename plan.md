# C++ Parser Surface Normalization Runbook

Status: Active
Source Idea: ideas/open/02_parser_cpp_surface_normalization_plan.md

## Purpose

Normalize the C++ parser so `final`, `override`, and `requires` are handled by
explicit grammar-driven paths instead of token-drain or identifier-spelling
special cases.

## Goal

Land small, regression-guarded parser slices that improve modern C++ syntax
acceptance without broadening into full semantic completeness.

## Core Rule

Prefer the smallest grammar-backed slice that can be validated with a reduced
test. Do not absorb adjacent parser initiatives such as lambda support or broad
`std::vector` bring-up work unless the active step directly requires it.

## Read First

- [ideas/open/02_parser_cpp_surface_normalization_plan.md](/workspaces/c4c/ideas/open/02_parser_cpp_surface_normalization_plan.md)
- [prompts/AGENT_PROMPT_EXECUTE_PLAN.md](/workspaces/c4c/prompts/AGENT_PROMPT_EXECUTE_PLAN.md)
- [src/frontend/lexer/token.hpp](/workspaces/c4c/src/frontend/lexer/token.hpp)
- [src/frontend/lexer/token.cpp](/workspaces/c4c/src/frontend/lexer/token.cpp)
- [src/frontend/parser/declarations.cpp](/workspaces/c4c/src/frontend/parser/declarations.cpp)
- [src/frontend/parser/statements.cpp](/workspaces/c4c/src/frontend/parser/statements.cpp)
- [src/frontend/parser/parser.hpp](/workspaces/c4c/src/frontend/parser/parser.hpp)

## Current Targets

- lexer keyword classification for the C++20 surface involved in this plan
- parser handling for contextual `final` / `override`
- parser handling for declaration `requires` clauses
- parser handling for `requires` expressions and constrained template syntax
- reduced tests under `tests/cpp/internal/postive_case/`

## Non-Goals

- full concepts semantic validation
- full misuse diagnostics for `final`
- broad override semantic completeness beyond the minimal validation needed for
  the first parser slice
- lambda parsing work from
  [ideas/open/03_parser_lambda_min_support_plan.md](/workspaces/c4c/ideas/open/03_parser_lambda_min_support_plan.md)
- unrelated `std::vector` compatibility work that does not depend directly on
  these parser surfaces

## Working Model

1. Reserve the standard keywords needed for this syntax family in the lexer when
   that removes parser-side string matching.
2. Add a reduced failing test before each parser change.
3. Replace skip-style parsing with dedicated parser entry points that follow the
   real syntax boundary.
4. Accept `final` and `requires` parser surfaces before enforcing full misuse
   diagnostics.
5. Add only the semantic validation needed to make `override` meaningful.

## Execution Rules

- Keep each patch tied to one reduced repro.
- Compare behavior against Clang when the intended syntax or parse shape is
  unclear.
- Prefer AST-neutral acceptance first when semantics are not yet required.
- Do not solve syntax support by consuming tokens until `;`, `{`, or the next
  declaration boundary.
- If execution uncovers a separate initiative, record it under `ideas/open/`
  instead of expanding this runbook.

## Ordered Steps

### Step 1: Normalize lexer keyword coverage

Goal: let the parser reason about targeted modern syntax via `TokenKind`
instead of raw identifier spelling.

Primary targets:

- [src/frontend/lexer/token.hpp](/workspaces/c4c/src/frontend/lexer/token.hpp)
- [src/frontend/lexer/token.cpp](/workspaces/c4c/src/frontend/lexer/token.cpp)
- reduced tests that prove the parser sees the intended token classes

Concrete actions:

- inspect current keyword classification for `final`, `override`, `requires`,
  and immediately adjacent words needed for the first parser slice
- add the narrowest reduced test that currently depends on identifier-text
  special casing
- promote only the keywords required for this initiative
- re-run the reduced parser coverage before widening scope

Completion check:

- the targeted reduced test passes with lexer-backed classification
- the parser no longer needs raw spelling checks for the covered slice

### Step 2: Add explicit parser support for contextual `final` and `override`

Goal: parse and retain class/function specifier surfaces without ad hoc token
drain behavior.

Primary targets:

- [src/frontend/parser/declarations.cpp](/workspaces/c4c/src/frontend/parser/declarations.cpp)
- [src/frontend/parser/parser.hpp](/workspaces/c4c/src/frontend/parser/parser.hpp)
- nearby reduced parser tests

Concrete actions:

- add reduced repros for the smallest unsupported `final` and `override` forms
- introduce parser helpers for these specifiers at the real declaration sites
- accept `final` first as a parser surface
- add only the minimal validation needed so an `override`-marked declaration is
  checked against a base virtual member when that path is wired

Completion check:

- reduced `final` and `override` tests pass
- skip-style handling for those surfaces is removed or no longer on the hot path

### Step 3: Replace `requires` skip paths with dedicated grammar entry points

Goal: accept C++20 `requires` syntax through explicit parsing.

Primary targets:

- [src/frontend/parser/declarations.cpp](/workspaces/c4c/src/frontend/parser/declarations.cpp)
- [src/frontend/parser/statements.cpp](/workspaces/c4c/src/frontend/parser/statements.cpp)
- [src/frontend/parser/parser.hpp](/workspaces/c4c/src/frontend/parser/parser.hpp)

Concrete actions:

- add one reduced repro at a time for declaration requires-clauses,
  requires-expressions, and constrained template parameters
- replace the existing skip-style bridge with dedicated parse paths
- keep the first implementation acceptance-oriented rather than semantically
  complete

Completion check:

- reduced `requires` tests pass without token-drain fallbacks
- parser recovery remains local and predictable on nearby malformed inputs

### Step 4: Reconnect to motivating library-facing repros

Goal: confirm the parser slices improve the upstream compatibility frontier
without claiming full standard-library support.

Primary targets:

- motivating reduced libstdc++ or `std::vector` repros that directly depend on
  these syntax families

Concrete actions:

- re-run the motivating repro after each completed syntax family
- record any surviving separate blocker as a bounded follow-up idea instead of
  mutating this plan

Completion check:

- the motivating repro advances past the normalized parser surface
- remaining failures, if any, are clearly outside this runbook’s scope

## Success Condition

The active syntax families are covered by explicit parser paths, reduced tests
protect them, and the parser relies less on identifier-spelling and skip-style
fallbacks for modern C++ surface syntax.
