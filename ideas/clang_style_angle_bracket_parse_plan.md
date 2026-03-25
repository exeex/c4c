# Clang-Style Angle Bracket Parse Plan

## Goal

Replace the current ad-hoc `<...>` / `>>` handling in the C++ parser with a
more Clang-like flow that relies on:

- context-sensitive template-id detection
- tentative parsing
- type-vs-expression disambiguation
- parser-owned `>` / `>>` splitting in template context
- angle-bracket tracking for recovery and diagnostics

This plan is intended as follow-up design work after the current EASTL-driven
local fixes in `src/frontend/parser/types.cpp`.


## Why This Exists

The current parser has several local repairs for nested template arguments and
dependent non-type template arguments, but the overall model is still too
token-scan-driven. That leads to recurring regressions around shapes like:

```cpp
A<B<C>>
Trait<T>::value
integral_constant<bool, Trait<T>::value>
foo<bar, baz>(x)
ns::template X<Y>::type
```

Recent EASTL failures showed that:

1. local angle-depth scanning can consume an outer `>` incorrectly
2. expression-vs-template ambiguities are being resolved too late
3. names inside deferred NTTP/default evaluation are not resolved using the
   same rules as normal template parsing
4. handling is spread across `parse_base_type`, template arg parsing, and
   fallback token skipping instead of a single parser-level template close
   policy


## Reference

Primary reference:

- [ref/llvm-project/clang/lib/Parse/ParseTemplate.cpp](/workspaces/c4c/ref/llvm-project/clang/lib/Parse/ParseTemplate.cpp)

Especially relevant areas:

- `ParseGreaterThanInTemplateList(...)`
- `ParseTemplateArgument()`
- `ParseTemplateArgumentList(...)`
- `checkPotentialAngleBracket(...)`
- `checkPotentialAngleBracketDelimiter(...)`

Supporting definitions:

- [ref/llvm-project/clang/include/clang/Parse/Parser.h](/workspaces/c4c/ref/llvm-project/clang/include/clang/Parse/Parser.h)


## Current State Summary

Today we already have one important prerequisite:

- lexer keeps `>`, `>>`, `>>>`, `>=`, `>>=` as distinct lexed tokens rather
  than eagerly splitting them in lexing

As of 2026-03-25, Stage 1 below is partially implemented in parser core:

- `src/frontend/parser/parse.cpp` now owns template-close consumption through
  `parse_greater_than_in_template_list(...)`
- `check_template_close()` / `match_template_close()` route through that
  parser-owned policy instead of each caller open-coding `>>` splitting
- a few existing template-parameter / struct-member close sites were migrated
  off manual `>> -> >` mutation

What this does not solve yet:

- canonical parsing of template argument contents
- expression-vs-type disambiguation for dependent NTTPs
- tentative parsing around potential template-id starts

Known remaining failing shape:

```cpp
integral_constant<bool, arithmetic<T>::value>
template <typename T, bool = is_arithmetic<T>::value>
```

That failure is expected to be addressed by Stage 2 and, if needed for
ambiguous expression contexts, Stage 3.

What is still missing is the parser-side policy that decides when a token
starting with `>` should be interpreted as:

- closing a template-argument-list
- part of another outer template close
- an operator token in an expression


## Design Direction

### 1. Centralize template-list closing

Add a parser helper modeled on Clang's `ParseGreaterThanInTemplateList(...)`.

Responsibilities:

- consume a plain `>`
- split `>>` into `>` plus remaining `>`
- split `>>>` into `>` plus remaining `>>`
- split `>=` into `>` plus remaining `=`
- split `>>=` into `>` plus remaining `>=`
- optionally leave the remainder token in the stream

This should become the only place where parser logic splits `>`-prefixed
tokens for template parsing.


### 2. Stop treating `<...>` as a generic token scan problem

Current code still has several "scan until comma / greater with angle_depth"
loops. Those should be treated as temporary recovery logic, not as primary
template parsing.

Target rule:

- if we are parsing a template-id, use a real template-argument parser
- if we are not sure whether `<` starts a template-id, use tentative parsing
- only fall back to token skipping for recovery after a parse failure


### 3. Disambiguate template arguments with parser rules, not heuristics

Adopt the Clang ordering for template arguments:

1. first try type-id
2. then try template-template argument
3. finally parse as non-type expression

This should replace the current practice of:

- peeking a few tokens
- guessing whether something is a type
- falling back to expression capture text too early

This matters especially for:

```cpp
X<Y<Z>>
integral_constant<bool, Trait<T>::value>
enable_if_t<(N > 0), int>
```


### 4. Introduce tentative parsing around potential template-id starts

When parser sees a possible template-name followed by `<`, do not immediately
commit to template parsing.

Instead:

1. record the candidate template-name and `<` location
2. tentatively parse a template-argument-list
3. if parse succeeds and closes legally, commit
4. otherwise revert and treat `<` as operator / expression syntax

This should be used in:

- expression parsing
- type parsing when a name might be either a type or value
- qualified-id parsing
- dependent-name handling


### 5. Add angle-bracket tracking for recovery

After the core parser path is stable, add an `AngleBracketTracker`-style helper.

Purpose:

- remember "this expression may have intended to be a template-name"
- improve recovery when later commas or `>` tokens strongly suggest a
  template-id
- improve diagnostics for malformed but template-like syntax

This is explicitly a second-phase improvement, not the first blocker.


## Proposed Implementation Stages

### Stage 0. Freeze current local fixes

Keep the recent local correctness fixes that:

- stop one known outer-`>` over-consumption path
- improve deferred NTTP default lookup in namespaced cases

Treat them as valid local fixes, not the final architecture.


### Stage 1. Add parser-owned template-close helper

Introduce a helper in parser core, for example:

```cpp
bool parse_greater_than_in_template_list(SourceLocation l_angle,
                                         SourceLocation* r_angle,
                                         bool consume_last_token);
```

Then migrate existing template list closing sites to use it.

Candidate sites:

- template parameter list parsing
- template argument list parsing
- any current `check_template_close()` / `match_template_close()` paths

Status on 2026-03-25:

- implemented in `src/frontend/parser/parse.cpp`
- `check_template_close()` now recognizes `>`, `>>`, `>=`, `>>=`
- `match_template_close()` now delegates to the parser-owned helper
- several existing local `>> -> >` repairs were migrated to the helper

Remaining gap for this stage:

- not every angle-depth recovery loop has been migrated yet
- there is still duplicated scanning logic outside the canonical close path


### Stage 2. Build a single template argument parser path

Create or refactor toward one canonical path for:

- parsing a template-id after a template-name
- parsing the template-argument-list contents
- filling `TypeSpec` / parsed template arg data structures

Avoid multiple partially-overlapping implementations in:

- `parse_base_type`
- qualified type parsing
- alias-template reconstruction
- deferred NTTP expression handling


### Stage 3. Add tentative parse wrapper for `<`

Create a reusable tentative parse utility for:

- "maybe template-id, maybe operator<"

This likely belongs near expression / type ambiguity handling rather than in
one EASTL-specific branch.

Expected effect:

- remove many `angle_depth` scans from expression-like contexts
- reduce accidental consumption of closing `>` tokens


### Stage 4. Normalize type-vs-expr template argument disambiguation

Make template argument parsing explicitly follow:

1. `try_parse_type_id_as_template_arg`
2. `try_parse_template_template_arg`
3. `parse_non_type_template_arg_expr`

This stage should also review any current uses of raw expression-text capture
for NTTPs and narrow them to only the cases where full parsing is not yet
supported.


### Stage 5. Add angle-bracket tracker and diagnostics

Once parsing is stable, add tracking for potential-but-uncommitted template
angle brackets to improve:

- diagnostics
- recovery after malformed template syntax
- suspicious expression cases like `foo<bar, baz>(x)`


## Code Areas To Audit

- [src/frontend/parser/types.cpp](/workspaces/c4c/src/frontend/parser/types.cpp)
- [src/frontend/parser/declarations.cpp](/workspaces/c4c/src/frontend/parser/declarations.cpp)
- [src/frontend/parser/expressions.cpp](/workspaces/c4c/src/frontend/parser/expressions.cpp)
- [src/frontend/parser/parse.cpp](/workspaces/c4c/src/frontend/parser/parse.cpp)
- [src/frontend/parser/parser.hpp](/workspaces/c4c/src/frontend/parser/parser.hpp)

Likely hotspots:

- `check_template_close()`
- `match_template_close()`
- template argument parsing inside `parse_base_type()`
- dependent-name handling with `::type`, `::value`
- alias-template application
- deferred NTTP default evaluation


## Acceptance Criteria

We should consider this plan successful when:

1. parser no longer relies on scattered angle-depth token scans as the primary
   mechanism for nested template syntax
2. `>`-prefixed tokens are split only by parser-owned template-close logic
3. `<` as template opener vs `<` as operator is decided via context plus
   tentative parsing
4. template arguments follow a consistent type-vs-expr parse order
5. EASTL/EABase regressions in this area are covered by focused tests, not only
   by one large integration repro


## Suggested Regression Buckets

Add reduced tests for at least these shapes:

```cpp
A<B<C>>
A<B<C<D>>>
integral_constant<bool, Trait<T>::value>
enable_if_t<(N > 0), int>
ns::X<Y>::type
ns::template X<Y>::type
foo<bar, baz>(x)
```

Also keep targeted EASTL-derived tests for:

- inherited trait `value` in template arguments
- dependent `::type` base clauses
- namespaced alias-template chains


## Non-Goals For The First Refactor

The first pass does not need to fully replicate every Clang recovery detail.

In particular, it is fine to defer:

- typo-oriented template diagnostics
- all C++20 concept-specific angle-bracket interactions
- full parity for every malformed nested template recovery path

The primary goal is correct parsing of valid C++17-style nested template syntax
without depending on brittle token scanning.
