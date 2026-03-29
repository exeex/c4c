# C++ Parser Lambda Minimal Support Plan

## Goal

Add baseline minimal parser support for C++ lambda expressions so the frontend
can consume simple lambda syntax through ordinary grammar-driven parsing.

## Why This Is Separate

Lambda parsing is adjacent to the broader parser surface normalization work, but
it expands into its own syntax family with distinct follow-on concerns:

- capture syntax
- call-operator shape
- optional parameter lists
- optional trailing return types
- interaction with expression parsing and AST representation

Keeping it separate prevents the active `override` / `final` / `requires` work
from quietly broadening into a second parser initiative.

## First-Wave Scope

Do:

- support the smallest useful lambda surface first
- prefer simple reduced parser repros
- keep semantics and lowering out of scope unless parse survival requires a tiny
  placeholder
- preserve lambda syntax as an explicit AST expression node instead of erasing
  it into a placeholder literal

Likely first-wave syntax:

- `[] { ... }`
- `[]() { ... }`
- `[&] { ... }`
- `[=] { ... }`

Do not:

- promise full lambda completeness
- block the first wave on generic lambdas, complex captures, trailing return
  types, or lowering behavior
- do not hoist lambda bodies into the translation unit function list during the
  parser phase
- mix this idea back into the active non-lambda parser normalization plan unless
  lifecycle priority changes

## Primary Targets

- [src/frontend/parser/expressions.cpp](/workspaces/c4c/src/frontend/parser/expressions.cpp)
- [src/frontend/parser/statements.cpp](/workspaces/c4c/src/frontend/parser/statements.cpp)
- [src/frontend/parser/parser.hpp](/workspaces/c4c/src/frontend/parser/parser.hpp)
- [src/frontend/parser/ast.hpp](/workspaces/c4c/src/frontend/parser/ast.hpp)
- reduced tests under `tests/cpp/internal/postive_case/`

## Intended Parser Shape

The first-wave implementation should model lambda as a real expression node,
not as a synthetic top-level function definition.

- add a dedicated `NK_LAMBDA` AST node
- parse and retain:
  - capture-default shape such as `[]`, `[&]`, `[=]`
  - a minimal explicit capture list when needed
  - optional parameter list
  - compound-statement body
  - optional trailing return type only if a concrete reduced repro requires it
- treat the lambda body as executing inside a lambda-local lexical scope /
  closure scope
- do not append the lambda body directly to the translation-unit function list
  during parsing

## Lowering Boundary

If later stages need executable lambda semantics, perform the desugaring after
parsing rather than during parsing.

That future lowering can introduce:

- a synthetic closure type
- a synthetic `operator()`
- optional function-pointer conversion support for captureless lambdas

But those are downstream concerns. The parser phase should only preserve enough
structure for later stages to make that choice safely.

## Validation Strategy

1. reduce the smallest unsupported lambda form into a standalone test
2. add the reduced test before parser changes
3. introduce the smallest dedicated lambda parse path that produces `NK_LAMBDA`
4. re-run nearby parser coverage
5. only then widen the accepted lambda surface if needed

## Success Condition

This idea is successful when simple lambda syntax has explicit parser support,
the parser preserves it as a lambda expression node instead of skipping it, and
a reduced regression test protects that surface.
