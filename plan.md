# C++17 Keyword Baseline Runbook

Status: Active
Source Idea: ideas/open/01_cpp17_keyword_support_plan.md

## Purpose

Turn the lexer's keyword coverage into a stable C++17-era baseline and patch the
minimum parser compatibility edges that break when those words stop lexing as
plain identifiers.

## Goal

Reserve the chosen C++17 keyword boundary in the lexer, keep nearby parser
surfaces stable, and land regression tests that pin the new token behavior.

## Core Rule

Keep this plan narrow. Do not expand into full C++20 syntax work, semantic
feature completion, lambda support, or unrelated standard-library bring-up
unless a small parser compatibility shim is directly required by the new
keyword tokens.

## Read First

- [ideas/open/01_cpp17_keyword_support_plan.md](/workspaces/c4c/ideas/open/01_cpp17_keyword_support_plan.md)
- [prompts/AGENT_PROMPT_EXECUTE_PLAN.md](/workspaces/c4c/prompts/AGENT_PROMPT_EXECUTE_PLAN.md)
- [src/frontend/lexer/token.hpp](/workspaces/c4c/src/frontend/lexer/token.hpp)
- [src/frontend/lexer/token.cpp](/workspaces/c4c/src/frontend/lexer/token.cpp)
- [src/frontend/parser/declarations.cpp](/workspaces/c4c/src/frontend/parser/declarations.cpp)
- [src/frontend/parser/expressions.cpp](/workspaces/c4c/src/frontend/parser/expressions.cpp)
- [src/frontend/parser/statements.cpp](/workspaces/c4c/src/frontend/parser/statements.cpp)
- [src/frontend/parser/types.cpp](/workspaces/c4c/src/frontend/parser/types.cpp)

## Current Targets

- lexer keyword inventory and classification in the frontend lexer
- parser sites that still depend on raw identifier spelling for newly reserved
  words
- lex regression coverage and a small number of nearby parser regressions under
  `tests/cpp/internal/`

## Non-Goals

- full C++20 keyword coverage
- broader `requires`, modules, or coroutines work
- lambda parsing or lowering
- semantic enforcement beyond what is needed to preserve existing covered parses
- unrelated `std::vector` or backend work

## Working Model

Implement this in small, test-backed slices:

1. inventory the existing keyword surface against the intended C++17 boundary
2. reserve missing keywords in `TokenKind` and lexer classification
3. patch parser hot spots that relied on identifier text for those words
4. add lex-only and targeted parse regressions
5. rerun nearby parser coverage and the full suite before handoff

If execution exposes adjacent but non-required C++20/parser work, record it as a
separate open idea instead of widening this runbook.

## Execution Rules

- Prefer `TokenKind`-driven parsing over raw identifier string checks where this
  plan touches parser logic.
- Add or update the narrowest validating test before implementation for each
  new keyword slice.
- Keep behavior-preserving changes separate from broader parser cleanup.
- Re-check the same source idea before adding new scope; this runbook owns only
  the C++17 keyword baseline.

## Ordered Steps

### Step 1: Inventory The Current Keyword Gap

Goal: produce a concrete list of missing C++17 keywords and alternative operator
spellings that should become reserved tokens in this plan.

Primary targets:

- [src/frontend/lexer/token.hpp](/workspaces/c4c/src/frontend/lexer/token.hpp)
- [src/frontend/lexer/token.cpp](/workspaces/c4c/src/frontend/lexer/token.cpp)

Actions:

- inspect the current `TokenKind` enum and keyword lookup tables
- compare them against the source idea's C++17 boundary
- note parser-sensitive words that currently rely on identifier spelling
- record the first implementation slice in `plan_todo.md`

Completion check:

- a bounded keyword inventory exists and the first lexer/test slice is chosen

### Step 2: Reserve The Missing Keywords In The Lexer

Goal: extend lexer token definitions and classification for the chosen C++17
keyword set without broadening beyond this plan's boundary.

Primary targets:

- [src/frontend/lexer/token.hpp](/workspaces/c4c/src/frontend/lexer/token.hpp)
- [src/frontend/lexer/token.cpp](/workspaces/c4c/src/frontend/lexer/token.cpp)

Actions:

- add missing `TokenKind` entries
- wire keyword text to token classification
- keep naming and ordering consistent with existing lexer conventions
- add or update lex-only tests for the new vocabulary before or alongside the
  implementation slice

Completion check:

- the lexer produces dedicated token kinds for the selected keyword slice and
  lex-focused tests cover the added words

### Step 3: Patch Parser Compatibility Hot Spots

Goal: preserve existing parse behavior where new reserved tokens would
otherwise break parser paths that assumed identifier text.

Primary targets:

- [src/frontend/parser/declarations.cpp](/workspaces/c4c/src/frontend/parser/declarations.cpp)
- [src/frontend/parser/expressions.cpp](/workspaces/c4c/src/frontend/parser/expressions.cpp)
- [src/frontend/parser/statements.cpp](/workspaces/c4c/src/frontend/parser/statements.cpp)
- [src/frontend/parser/types.cpp](/workspaces/c4c/src/frontend/parser/types.cpp)

Actions:

- search for identifier-text special cases involving newly reserved words
- replace or augment them with `TokenKind`-aware handling
- keep fixes local to the compatibility edge exposed by the new tokens
- avoid turning this step into a generic parser normalization pass

Completion check:

- targeted covered parses continue to work after the token-kind changes

### Step 4: Lock In Regressions

Goal: pin both the lexer boundary and the nearby parser compatibility edges that
this plan intentionally changes.

Primary targets:

- `tests/cpp/internal/`

Actions:

- add a dedicated `--lex-only` regression for the new keyword vocabulary
- add reduced parse regressions for affected words such as `typename`,
  `noexcept`, `nullptr`, access labels, `final`, or nearby equivalents when
  the token-kind change matters
- keep each new test attached to one concrete lexer or parser edge

Completion check:

- the new vocabulary and the known parser-sensitive edges are covered by reduced
  committed tests

### Step 5: Validate And Triage Follow-Ons

Goal: prove the slice is stable and keep adjacent work out of scope.

Actions:

- run the targeted tests for the keyword slice
- rerun nearby parser/frontend coverage
- run the full `ctest` suite according to the execution prompt
- record blockers or follow-on ideas in `plan_todo.md` or `ideas/open/` when
  the work is adjacent but not required for this runbook

Completion check:

- targeted tests pass, full-suite status is recorded, and any remaining
  out-of-scope work is documented without being silently absorbed here
