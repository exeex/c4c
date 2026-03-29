# C++ Parser Surface Normalization Plan

## Goal

Normalize the C++ parser so it can consume modern syntax such as contextual
`final` / `override` and C++20 `requires` through ordinary grammar-driven
parsing instead of ad hoc string-skipping or token-drain hacks.

For semantic depth, treat these syntax families differently:

- `override` needs real validation logic
- `final` should parse and be accepted first; enforcement can come later with
  negative tests
- `requires` should parse and be accepted first; constraint checking can come
  later with negative tests

## Why This Now

Recent `std::vector` bring-up work proved that several libstdc++ blockers are no
longer about one isolated header quirk. They are exposing a broader parser-shape
problem:

- some modern C++ syntax is only tolerated through narrow skip logic
- some constructs are currently parsed by consuming tokens until the next
  plausible declaration boundary
- this makes parser state, recovery quality, and follow-on syntax support less
  predictable than they need to be

This is now important enough to treat as its own initiative rather than keep
burying it inside one standard-library bring-up.

Update:

The near-term first slice has shifted slightly earlier in the frontend pipeline.
Before more parser cleanup, the lexer should carry a fuller C++20 keyword
vocabulary so the frontend can stop relying on large amounts of identifier-text
special-casing for standard syntax.

## Primary Targets

- [src/frontend/lexer/token.hpp](/workspaces/c4c/src/frontend/lexer/token.hpp)
- [src/frontend/lexer/token.cpp](/workspaces/c4c/src/frontend/lexer/token.cpp)
- [src/frontend/parser/declarations.cpp](/workspaces/c4c/src/frontend/parser/declarations.cpp)
- [src/frontend/parser/statements.cpp](/workspaces/c4c/src/frontend/parser/statements.cpp)
- [src/frontend/parser/parser.hpp](/workspaces/c4c/src/frontend/parser/parser.hpp)
- reduced tests under `tests/cpp/internal/postive_case/`

## Concrete Syntax Frontiers

The first syntax families to normalize are:

- contextual class/function specifiers:
  - `final`
  - `override`
- C++20 constraints syntax:
  - declaration requires-clause
  - requires-expression
  - constrained template parameters

## Semantic Staging

This initiative is primarily parser-first, but not every syntax family should
stop at the same depth.

Before those parser slices, the lexer may need to reserve the corresponding
standard keywords so the parser can reason over `TokenKind` instead of raw text.

- `override`
  - implement the parser surface and the basic validation logic needed to check
    that an override-marked declaration actually overrides a base virtual member
- `final`
  - treat as a contextual specifier that parses and is accepted
  - do not block this initiative on enforcing misuse diagnostics yet
- `requires`
  - treat requires-clause / requires-expression syntax as accepted parser
    surfaces first
  - do not block this initiative on concept satisfaction or misuse diagnostics
    yet

## Rules

Do:

- promote standard C++20 reserved words into lexer keyword classification when
  that reduces parser-side string matching
- introduce parser entry points that correspond to real grammar surfaces
- keep each slice tied to a reduced committed repro test
- prefer bounded AST-neutral parsing support first when semantics are not needed
- make recovery behavior more local and predictable as grammar support improves
- distinguish parser acceptance from semantic enforcement so `final` and
  `requires` do not delay the higher-value parser work

Do not:

- rely on string matching to classify whole constructs when token structure is
  available
- solve new syntax by blindly consuming until `;`, `{`, or the next declaration
- broaden this plan into semantic completeness for concepts
- broaden `final` or `requires` support into full misuse diagnostics in the
  first wave
- mix unrelated `std::vector` compatibility work into this initiative unless it
  directly depends on the parser surface being normalized

## Current Evidence

This direction is motivated by already-exposed syntax families from recent
libstdc++ work:

- record `final` syntax already needed a reduced repro
- `requires` support currently includes at least one skip-style bridge in
  [src/frontend/parser/declarations.cpp](/workspaces/c4c/src/frontend/parser/declarations.cpp)
- the lexer still reserved only a narrow C++ keyword subset, forcing multiple
  parser paths to special-case standard words by identifier spelling
- lambda expressions have been split into
  [ideas/open/parser_lambda_min_support_plan.md](/workspaces/c4c/ideas/open/parser_lambda_min_support_plan.md)
  so this plan stays tightly scoped

## Validation Strategy

For each syntax family:

1. reproduce the current failure or missing-grammar shape with the smallest
   standalone test
2. add the reduced test to the suite before parser changes
3. replace skip logic with a dedicated parse path for that syntax family
4. re-run the reduced tests plus nearby parser coverage
5. only then re-check the motivating `std::vector` or libstdc++ repro

## Success Condition

This idea is successful when the parser has explicit, reusable grammar paths for
the targeted syntax families, the old ad hoc fallback logic has been reduced or
retired where safe, and reduced regression tests protect those surfaces.
