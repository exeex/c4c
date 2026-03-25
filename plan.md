# Clang-Style Angle Bracket Parse Execution Plan

## Purpose

This document is for an execution-oriented AI agent.

Read it as a runbook.
Do not redesign the whole parser.
Do not try to reach full Clang recovery parity in one diff.
Do not scatter new local `<...>` fixes across unrelated parser code.


## One-Sentence Goal

Finish the parser refactor from:

- ad-hoc `<...>` / `>>` handling plus local angle-depth scanning

to:

- parser-owned template-close handling
- canonical template-argument parsing
- tentative parsing for `<` ambiguity
- explicit type-vs-expression template-argument disambiguation

while keeping valid existing C++ template syntax working.


## Current Reality

This repo is already partway through the refactor.

These pieces already exist:

1. parser-owned template-close helper support in
   [parse.cpp](/workspaces/c4c/src/frontend/parser/parse.cpp)
2. parser declarations in
   [parser.hpp](/workspaces/c4c/src/frontend/parser/parser.hpp)
3. current type parsing and template-id handling in
   [types.cpp](/workspaces/c4c/src/frontend/parser/types.cpp)
4. current expression parsing in
   [expressions.cpp](/workspaces/c4c/src/frontend/parser/expressions.cpp)
5. current declaration parsing in
   [declarations.cpp](/workspaces/c4c/src/frontend/parser/declarations.cpp)
6. reference implementation ideas in
   [ParseTemplate.cpp](/workspaces/c4c/ref/llvm-project/clang/lib/Parse/ParseTemplate.cpp)

What is still wrong:

- template argument parsing is not yet owned by one canonical parser path
- dependent non-type template arguments still regress in some cases
- `<` as template opener vs operator is still resolved too locally
- angle-depth scanning still exists as primary behavior in some paths
- parser recovery and diagnostics still do not have a unified angle-bracket policy


## Files You Will Touch

Primary files:

1. [types.cpp](/workspaces/c4c/src/frontend/parser/types.cpp)
- canonicalize template argument parsing here
- reduce duplicated local `<...>` logic

2. [expressions.cpp](/workspaces/c4c/src/frontend/parser/expressions.cpp)
- add tentative parse entry points where `<` is ambiguous
- keep expression semantics here, not in low-level token scanning

3. [parse.cpp](/workspaces/c4c/src/frontend/parser/parse.cpp)
- keep parser-owned `>` / `>>` splitting policy here
- extend only when a shared parser primitive is needed

4. [parser.hpp](/workspaces/c4c/src/frontend/parser/parser.hpp)
- declare parser helpers and small result structs
- do not turn this into a dumping ground for ad-hoc state

5. [declarations.cpp](/workspaces/c4c/src/frontend/parser/declarations.cpp)
- only touch if a declaration-side template parse call path still bypasses the
  canonical helper

Do not start in:

- lexer files
- AST/HIR lowering files
- codegen files
- unrelated C or non-template parser features


## Current Entry Points

Before editing, read these exact points:

1. parser-owned template-close policy:
- [parse.cpp](/workspaces/c4c/src/frontend/parser/parse.cpp)

2. parser declarations:
- [parser.hpp](/workspaces/c4c/src/frontend/parser/parser.hpp)

3. current template type parsing:
- [types.cpp](/workspaces/c4c/src/frontend/parser/types.cpp)

4. current expression primary parsing:
- [expressions.cpp](/workspaces/c4c/src/frontend/parser/expressions.cpp)

5. current declaration parsing:
- [declarations.cpp](/workspaces/c4c/src/frontend/parser/declarations.cpp)

6. Clang reference for target behavior:
- [ParseTemplate.cpp](/workspaces/c4c/ref/llvm-project/clang/lib/Parse/ParseTemplate.cpp)


## Reference Mapping

Primary reference:

- [ParseTemplate.cpp](/workspaces/c4c/ref/llvm-project/clang/lib/Parse/ParseTemplate.cpp)

Supporting definitions:

- [Parser.h](/workspaces/c4c/ref/llvm-project/clang/include/clang/Parse/Parser.h)

Treat these Clang entry points as structural guides, not copy-paste targets:

1. `ParseGreaterThanInTemplateList(...)`
- ownership target for parser-side `>` / `>>` / `>=` splitting
- this maps to our shared template-close helper work in
  [parse.cpp](/workspaces/c4c/src/frontend/parser/parse.cpp)

2. `ParseTemplateArgument()`
- ownership target for parsing exactly one template argument with explicit
  type-vs-template-vs-expression ordering
- this maps to our canonical single-argument template parse logic, primarily in
  [types.cpp](/workspaces/c4c/src/frontend/parser/types.cpp)

3. `ParseTemplateArgumentList(...)`
- ownership target for parsing the whole `<...>` list through one canonical path
- this maps to our shared template-argument-list entry point and all callers
  that currently inline their own loops

4. `checkPotentialAngleBracket(...)`
- ownership target for detecting "this expression may actually have been meant
  as a template-id" and for recovery-oriented handling of angle brackets
- this maps to our later-stage tentative parsing and recovery tracking work,
  primarily in expression parsing

5. `checkPotentialAngleBracketDelimiter(...)`
- ownership target for validating whether a later token still fits the
  "potential template-id" interpretation
- this maps to our late-stage angle-bracket tracker / delimiter checks, not to
  the first canonical parsing slice

Use this mapping while planning each diff:

- if you are changing template-close consumption, think
  `ParseGreaterThanInTemplateList(...)`
- if you are changing single-argument classification, think
  `ParseTemplateArgument()`
- if you are changing shared `<...>` parsing, think
  `ParseTemplateArgumentList(...)`
- if you are changing ambiguous `<` recovery or suspicious delimiter handling,
  think `checkPotentialAngleBracket(...)` and
  `checkPotentialAngleBracketDelimiter(...)`


## What “Done” Means

This track is complete when all of these are true:

1. parser-owned helper is the only place that splits `>`-prefixed tokens for
   template closing
2. template argument parsing goes through one canonical parser path
3. `<` as template opener vs operator is decided by parser context and
   tentative parsing, not mostly by token scans
4. template argument disambiguation follows:
   - type-id first
   - template-template argument second
   - non-type expression last
5. focused regression cases for nested templates and dependent NTTPs stay green


## Rules For The Agent

Follow these rules while implementing:

1. Make one parser ownership change per diff.
2. Preserve valid parsing behavior before generalizing recovery.
3. If a decision is about token spelling or `>` remainder splitting, keep it in
   parser core helpers.
4. If a decision is about whether something is a type, template, or
   expression, keep it in the parsing layer that owns that ambiguity.
5. Do not add more angle-depth scanning as a new primary mechanism.
6. Only add recovery-oriented tracking after the canonical parse path is stable.


## Current Problem To Solve

The real problem is not “we need one more local fix for `>>`”.

The real problem is:

- template-id parsing does not have one canonical parser path
- type-vs-expression template argument handling is inconsistent
- parser ownership for `<` / `>` ambiguity is still incomplete
- local token scans are still doing work that should belong to real parsing

Your job is to finish the ownership split inside the parser, not to patch one
specific EASTL example in isolation.


## Implementation Order

Do the work in this order.
Do not skip ahead.


## Step 1. Audit Remaining Angle-Bracket Ownership Gaps

Goal:

- identify exactly where parser code still bypasses the canonical close policy
  or uses local angle-depth scanning as primary behavior

Do this:

1. open
   [parse.cpp](/workspaces/c4c/src/frontend/parser/parse.cpp)
2. open
   [types.cpp](/workspaces/c4c/src/frontend/parser/types.cpp)
3. open
   [expressions.cpp](/workspaces/c4c/src/frontend/parser/expressions.cpp)
4. list every remaining use of:
   - angle-depth scanning loops
   - manual `>` / `>>` / `>=` handling
   - inline template-argument parsing outside the canonical path
   - local heuristics for type-vs-expression template args
5. group each dependency into one of:
   - close-policy ownership gap
   - canonical template-argument parser gap
   - tentative parsing gap
   - recovery-only logic that is acceptable for now
6. for each gap, map it to the intended Clang structural reference:
   - `ParseGreaterThanInTemplateList(...)`
   - `ParseTemplateArgument()`
   - `ParseTemplateArgumentList(...)`
   - `checkPotentialAngleBracket(...)`
   - `checkPotentialAngleBracketDelimiter(...)`

Completion check:

- you can point to the exact remaining ownership gaps that keep this refactor
  incomplete
- you can say which Clang structure each remaining gap is supposed to match


## Step 2. Finish Parser-Owned Template Close Handling

Goal:

- every legitimate template-list close path should use one parser-owned helper

Clang structure to follow:

- `ParseGreaterThanInTemplateList(...)`

Do this:

1. choose one remaining caller that still handles template close locally
2. route it through the canonical parser helper in
   [parse.cpp](/workspaces/c4c/src/frontend/parser/parse.cpp)
3. keep token remainder behavior correct for:
   - `>`
   - `>>`
   - `>>>`
   - `>=`
   - `>>=`
4. keep behavior the same except for removing duplicated local logic

Do not do in one diff:

- all remaining callers at once
- diagnostics overhaul
- type-vs-expression redesign

Completion check:

- no touched caller splits `>`-prefixed tokens itself anymore
- the touched behavior is converging on a single
  `ParseGreaterThanInTemplateList(...)`-style ownership point


## Step 3. Build One Canonical Template Argument Parser Path

Goal:

- stop parsing template arguments through multiple partially-overlapping paths

Clang structures to follow:

- `ParseTemplateArgumentList(...)`
- `ParseTemplateArgument()`

Read first:

- [types.cpp](/workspaces/c4c/src/frontend/parser/types.cpp)
- [parser.hpp](/workspaces/c4c/src/frontend/parser/parser.hpp)

Do this:

1. identify the canonical entry point for template argument parsing
2. move one real caller from inline parsing to that canonical entry point
3. make the list parser own comma/close sequencing the way
   `ParseTemplateArgumentList(...)` does
4. make the single-argument parser own one-argument classification the way
   `ParseTemplateArgument()` does
5. make the shared result structure carry what downstream code actually needs
6. keep nested template-id parsing going through the same path
7. preserve valid cases before broadening unsupported NTTP expressions

Prioritize these before cosmetic cleanup:

- nested type template arguments
- dependent `Trait<T>::value`-style NTTPs
- alias-template reconstruction callers
- expression-side template-id parsing currently duplicating logic

Completion check:

- the touched caller no longer implements its own template-argument mini-parser
- the touched path now has a clear split between
  `ParseTemplateArgumentList(...)`-style list ownership and
  `ParseTemplateArgument()`-style per-argument ownership


## Step 4. Normalize Template Argument Disambiguation Order

Goal:

- parse template arguments by parser rules, not scattered guesses

Clang structure to follow:

- `ParseTemplateArgument()`

Do this:

1. make one canonical template-argument parse path follow this order:
   - try type-id
   - try template-template argument
   - parse non-type expression
2. only fall back to raw capture or token skipping for recovery
3. ensure dependent names such as `Trait<T>::value` can survive as NTTP input
4. keep bool / literal / `sizeof...(Pack)` handling working

Do not do in one diff:

- all deferred consteval forwarding repairs
- unrelated declaration parsing cleanup

Completion check:

- the touched code path no longer decides type-vs-expr mostly by peeking a few
  tokens and guessing
- the touched code path now reads like a local analogue of
  `ParseTemplateArgument()`


## Step 5. Add Tentative Parsing Around Potential Template-Id Starts

Goal:

- decide `<` as template opener vs operator by tentative parsing when context is
  ambiguous

Clang structures to follow:

- `checkPotentialAngleBracket(...)`
- later, when needed, `checkPotentialAngleBracketDelimiter(...)`

Do this:

1. identify one ambiguous parse site, usually in expression parsing
2. add a reusable tentative parse wrapper there
3. if template-id parsing succeeds and closes legally, commit
4. otherwise revert and continue as expression/operator parsing
5. keep this mechanism reusable rather than EASTL-specific

Good candidates:

- expression primary parsing
- qualified-id parsing
- dependent-name parsing

Do not do in one diff:

- every ambiguous site at once
- angle-bracket diagnostics tracker
- full malformed-template recovery policy

Completion check:

- the touched ambiguous site no longer relies primarily on angle-depth scanning
  to decide whether `<` was a template opener
- the touched site now has an explicit "potential template-id" path in the same
  spirit as `checkPotentialAngleBracket(...)`


## Step 6. Add Recovery/Diagnostics Tracking Only After Core Parsing Is Stable

Goal:

- improve malformed-template recovery only after valid-template parsing is owned
  by the canonical path

Clang structures to follow:

- `checkPotentialAngleBracket(...)`
- `checkPotentialAngleBracketDelimiter(...)`

Do this:

1. add an angle-bracket tracking helper only if the core parser path is already
   stable
2. use it to improve:
   - diagnostics
   - recovery after malformed template syntax
   - suspicious `foo<bar, baz>(x)`-style cases
3. keep it as recovery support, not as the main parser mechanism

Completion check:

- recovery support improves diagnostics without becoming a replacement for real
  parsing
- delimiter checks and suspicious-angle tracking are clearly secondary helpers,
  analogous to `checkPotentialAngleBracket(...)` /
  `checkPotentialAngleBracketDelimiter(...)`, not the main parser path


## How To Decide Whether Logic Belongs In Parser Core Or In A Specific Parse Path

Use this rule:

1. If it answers “how should `>` / `>>` / `>=` be consumed in template
   context?”:
- parser core helper

2. If it answers “is this a type, template, or expression argument?”:
- template argument parser

3. If it answers “is `<` opening a template-id or acting as an operator here?”:
- tentative parsing in the owning parse path

4. If it exists only to skip tokens after a failure:
- recovery logic, and it should not become the main path


## Special Cases To Migrate Late

Leave these for later slices unless they are your explicit target:

- malformed template diagnostics
- full Clang-style recovery parity
- exotic operator interactions such as `operator<=>` ambiguity
- every deferred consteval NTTP forwarding corner at once

These are high-risk because they can hide ownership mistakes behind recovery.


## Minimum Validation Strategy

After each meaningful diff:

1. build the parser target or the main project build that covers parser changes
2. run focused parser/regression tests covering:
   - nested templates
   - dependent NTTPs
   - qualified `::type` / `::value`
   - template-vs-expression ambiguity
3. if a regression appears, classify it as:
   - real parser ownership bug
   - missing supported NTTP expression form
   - recovery-only limitation

Suggested focus corpus:

- `A<B<C>>`
- `A<B<C<D>>>`
- `integral_constant<bool, Trait<T>::value>`
- `template <typename T, bool = is_arithmetic<T>::value>`
- `enable_if_t<(N > 0), int>`
- `ns::X<Y>::type`
- `ns::template X<Y>::type`
- `foo<bar, baz>(x)`


## Good First Diff

A good first diff does exactly this:

1. audits one real remaining template-close ownership gap
2. moves that caller onto the parser-owned close helper
3. preserves behavior on valid nested template syntax
4. leaves broader tentative parsing work alone


## Good Second Diff

A good second diff does exactly this:

1. replaces one inline template-argument parser with the canonical shared path
2. keeps nested template arguments working
3. proves focused regression cases still parse correctly


## Abort Conditions

Stop and reassess if either of these happens:

1. you need lexer redesign just to continue
2. you are adding more local angle-depth scanning to fix regressions caused by
   missing parser ownership

If that happens:

- make the smallest safe extraction
- update this plan
- do not silently expand the project scope


## Short Version

If you only remember one sequence, remember this:

1. audit remaining local angle handling
2. finish parser-owned template close consumption
3. move callers onto one canonical template-argument parser
4. normalize type-vs-expression disambiguation
5. use tentative parsing for `<` ambiguity
6. add recovery tracking only after valid parsing is stable
