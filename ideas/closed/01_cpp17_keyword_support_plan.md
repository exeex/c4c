# C++17 Keyword Support Plan

Status: Complete

## Completion Summary

Completed on 2026-03-29.

This idea delivered the planned C++17 keyword-baseline work across the lexer
and the minimum parser-compatibility surfaces needed to keep covered parses
stable. The completed slices reserved `typename`, `noexcept`, `nullptr`,
`friend`, `virtual`, `final`, `mutable`, `public` / `protected` / `private`,
and the alternative operator spellings `and`, `or`, `not`, `not_eq`,
`bitand`, `bitor`, `bitxor`, and `compl`, with reduced regression coverage
added under `tests/cpp/internal/`.

## Leftover Issues

- Assignment-form aliases such as `and_eq`, `or_eq`, and `xor_eq` were left
  out of scope to keep this runbook narrow. If they are still needed, they
  should land as a separate follow-on idea instead of reopening this one.
- Some record-typed overloaded-operator runtime gaps surfaced while building
  parser-boundary regressions for `and` and `not_eq`; those backend lowering
  issues remain separate from this completed lexer/parser baseline.

## Goal

Raise the frontend's keyword support baseline to a solid C++17 level before
attempting broader C++20 keyword or syntax expansion.

The intent is to improve lexer/parser stability with low migration cost, not to
promise immediate semantic completeness for every C++17 feature.

## Why This Direction

A recent attempt to jump straight toward a fuller C++20 keyword vocabulary
proved too disruptive for the current codebase state.

The lower-risk path is:

- first normalize the lexer around a complete or near-complete C++17 keyword set
- then remove parser-side identifier-text special cases where the new keyword
  tokens make that easy
- only after that revisit the C++20-specific surfaces such as `requires`,
  modules, and coroutines

This keeps the migration cost smaller and gives later parser work a steadier
base.

## Scope

This idea is limited to the C++17 keyword baseline and the minimum parser
compatibility work needed to keep existing behavior stable after those words
become reserved tokens.

Do:

- audit the current lexer keyword vocabulary against the C++17 keyword set
- add the missing C++17 keywords to lexer classification
- treat alternative operator spellings such as `and`, `or`, and `not` as part
  of that keyword-level vocabulary decision
- patch parser hot spots that currently depend on identifier spelling for newly
  reserved C++17 words
- add lex-only regression coverage plus a few nearby parse regressions where the
  token-kind change is known to matter

Do not:

- broaden this idea into full C++20 keyword coverage
- fold `requires`, modules, or coroutines into the first implementation wave
- promise semantic support for all syntax associated with the new keywords
- mix lambda lowering, `override` validation, or virtual dispatch work into this
  idea unless a tiny parser compatibility shim is required

## Suggested Keyword Boundary

Prioritize the C++17-era reserved-word set and contextual parser-sensitive
spelling first, including areas such as:

- `typename`
- `noexcept`
- `nullptr`
- `friend`
- `virtual`
- `final`
- `override`
- `mutable`
- `public`, `protected`, `private`
- `decltype`
- `constexpr`, `consteval` compatibility paths already present nearby
- alternative operator spellings:
  - `and`, `and_eq`
  - `bitand`, `bitor`
  - `compl`
  - `not`, `not_eq`
  - `or`, `or_eq`
  - `xor`, `xor_eq`

Leave explicitly C++20-first words for later follow-up unless they are already
harmless to reserve and do not expand parser scope.

## Primary Targets

- [src/frontend/lexer/token.hpp](/workspaces/c4c/src/frontend/lexer/token.hpp)
- [src/frontend/lexer/token.cpp](/workspaces/c4c/src/frontend/lexer/token.cpp)
- [src/frontend/parser/declarations.cpp](/workspaces/c4c/src/frontend/parser/declarations.cpp)
- [src/frontend/parser/expressions.cpp](/workspaces/c4c/src/frontend/parser/expressions.cpp)
- [src/frontend/parser/statements.cpp](/workspaces/c4c/src/frontend/parser/statements.cpp)
- [src/frontend/parser/types.cpp](/workspaces/c4c/src/frontend/parser/types.cpp)
- lex regression coverage under `tests/cpp/internal/`

## Execution Shape

Recommended implementation order:

1. inventory the current lexer vocabulary against the desired C++17 boundary
2. add missing `TokenKind` entries and keyword classification
3. update parser sites that still check raw identifier text for those words
4. add a dedicated `--lex-only` regression file for the new vocabulary
5. re-run nearby parse regressions that touch `typename`, `noexcept`,
   `nullptr`, access labels, `final`, and similar surfaces
6. record any newly exposed C++20-only follow-ons as separate ideas instead of
   expanding this one

## Handoff Notes

This idea is intentionally written as a handoff plan for later implementation.

Whoever picks it up should treat success as:

- the lexer reliably recognizing the chosen C++17 keyword boundary
- the parser continuing to accept existing covered cases after those words stop
  being plain identifiers
- the repo gaining regression tests that pin the new vocabulary and the most
  obvious compatibility edges

Anything beyond that should be split into a narrower follow-up idea rather than
quietly folded into this one.
