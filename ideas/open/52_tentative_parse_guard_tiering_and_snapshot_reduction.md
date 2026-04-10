# Tentative Parse Guard Tiering And Snapshot Reduction

Status: Open
Last Updated: 2026-04-10

## Goal

Reduce parser-side speculative parsing cost by splitting tentative parsing into
cheap and heavy paths, so common disambiguation cases stop paying full parser
state snapshot cost on every probe.

The immediate objective is not to remove `TentativeParseGuard` or force the
parser into a Clang-style "token rewind only" architecture. It is to preserve
today's debuggability and safety while moving the 90% common path off the
current whole-state snapshot model.

## Why This Idea Exists

Recent EASTL parse profiling made the cost shape visible:

- `--pp-only` on `eastl_vector_simple.cpp` was roughly `0.05s`
- `--lex-only` was roughly `0.08s`
- `--parse-only` was roughly `12s`

That isolated the problem to parser work rather than preprocessing or lexing.

An additional A/B experiment with `ENABLE_HEAVY_TENTATIVE_SNAPSHOT` made the
tentative parsing cost much more concrete:

- heavy snapshot `ON`: roughly `11.6s`
- heavy snapshot `OFF`: roughly `2.4s`

This is not a small micro-optimization. It indicates that speculative parsing
currently spends a large fraction of its total time copying and restoring
parser semantic state.

## Current Problem Shape

`TentativeParseGuard` is currently used as a general-purpose speculative parse
transaction. It is not limited to template code.

Representative hot-path usage exists in:

- template argument type-vs-non-type parsing
- `sizeof(type)` vs `sizeof(expr)` disambiguation
- `if (...)` declaration-condition probing
- range-for vs regular-for probing
- local declaration vs constructor-style initialization probing
- assorted struct/member/declarator ambiguity handling

Today the guard snapshots a heavy `ParserSnapshot`, including state such as:

- `typedefs_`
- `user_typedefs_`
- `typedef_types_`
- `var_types_`
- `last_resolved_typedef_`
- `template_arg_expr_depth_`
- token mutation checkpoints

That makes each tentative "push/pop" much more expensive than the syntax
question being asked in many common cases.

## Main Objective

Introduce a tiered tentative parsing model:

- keep a heavy guard for correctness-critical speculative parses that may touch
  semantic environment state
- add a lighter guard for common lookahead/disambiguation paths that only need
  cheap parser-state rewind
- leave room for a future journaled rollback model without forcing that
  migration in the first slice

Success means:

- common-path tentative parsing no longer performs full copies of typedef/type
  maps and variable bindings
- parser performance on heavy C++ header workloads improves materially
- correctness-sensitive paths still have an explicit safe fallback
- the codebase remains debug-friendly and does not hide rollback behavior

## Proposed Direction

### 1. Introduce A Lite Tentative Guard

Add a second guard, tentatively shaped like:

- `TentativeParseGuardLite`

The lite guard should rewind only cheap parser state, such as:

- `pos_`
- `last_resolved_typedef_`
- `template_arg_expr_depth_`
- token mutation checkpoint / token cursor state
- any small delimiter or nesting counters required for correctness

It should explicitly *not* snapshot heavy semantic environment containers like:

- `typedefs_`
- `user_typedefs_`
- `typedef_types_`
- `var_types_`

### 2. Keep The Heavy Guard

Retain today's heavy `TentativeParseGuard` for cases where speculative parsing
can mutate semantic environment state or where safety has not yet been proven.

This idea is intentionally not proposing a hard cut-over to a single minimal
rewind mechanism.

### 3. Classify Call Sites

Audit current tentative parsing call sites and split them into:

- likely-safe lite disambiguation probes
- still-heavy semantic probes
- ambiguous sites that need local review or instrumentation

Likely first-wave lite candidates:

- `sizeof(type)` vs `sizeof(expr)`
- declaration-condition probing in `if`
- range-for probing
- constructor-style initialization vs function-declaration lookahead

Likely heavy candidates at first:

- template arg type-vs-value parsing
- dependent/typedef-heavy declarator probes
- struct/member/default-argument probes that can touch type environment state

### 4. Add Instrumentation Before Deeper Surgery

Add lightweight counters or summary diagnostics for:

- tentative guard enter/commit/rollback counts
- heavy guard usage count
- lite guard usage count
- average or total snapshot payload sizes for heavy guards

The first optimization passes should be guided by measured hot spots, not only
by intuition.

### 5. Leave Room For A Journaled Future

If tiering delivers most of the benefit, that may be enough for near-term
performance goals.

If more headroom is needed, a later slice can replace heavy whole-state
snapshotting with a mutation journal / checkpoint design:

- enter tentative parse by recording a checkpoint
- append semantic mutations to a rollback log
- revert only the delta rather than restoring copied containers

That future work should remain optional until call-site tiering has been tried.

## Why Not Fully Copy LLVM/Clang

LLVM/Clang is useful as a reference point, but it is not the exact target here.

Clang's tentative parsing primarily rewinds token/preprocessor state and a
small amount of parser bookkeeping. That is excellent for performance, but it
also relies on tighter discipline around speculative parser side effects.

For this codebase, there is value in keeping a more explicit and debuggable
heavy guard available while gradually shrinking its usage surface.

The intended strategy is:

- borrow the "cheap path for common cases" idea
- avoid an all-at-once semantic purity rewrite
- preserve a readable, inspectable rollback model during migration

## Primary Scope

- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/parser_support.cpp`
- `src/frontend/parser/parser_expressions.cpp`
- `src/frontend/parser/parser_statements.cpp`
- `src/frontend/parser/parser_declarations.cpp`
- `src/frontend/parser/parser_types_declarator.cpp`
- `src/frontend/parser/parser_types_struct.cpp`
- any nearby parser instrumentation or test harness files needed for profiling

## Non-Goals

- no removal of rollback safety from correctness-critical speculative parses
- no Release-only parser semantics changes
- no unconditional replacement of heavy rollback with token rewind alone
- no broad parser architecture rewrite hidden inside a performance slice
- no reliance on exceptions or build modes as the primary optimization strategy

## Required Migration Strategy

1. add `TentativeParseGuardLite` and the supporting lightweight snapshot data
2. add counters or summaries to understand guard usage mix
3. migrate the safest common-path call sites first
4. validate correctness on focused parser regressions plus existing EASTL cases
5. re-measure parser-heavy workloads after each migration wave
6. only then decide whether journaled heavy rollback is worth the next slice

## Validation

At minimum, validation should include:

- `tests/cpp/eastl/eastl_vector_simple.cpp` timing checks in `Release`
- the existing EASTL parse/workflow recipes
- parser regressions covering:
  - `sizeof(type)` vs `sizeof(expr)`
  - range-for parsing
  - `if` declaration conditions
  - template arg type-vs-value parsing
  - constructor-style init vs function declaration ambiguity
- spot checks with `ENABLE_HEAVY_TENTATIVE_SNAPSHOT=ON` and `OFF`

Success should be judged by both:

- preserved parser correctness
- measurable reduction in parser-only wall time on template-heavy inputs

## Current Performance Baseline

The primary benchmark file for this idea should be:

- [eastl_vector_simple.cpp](/workspaces/c4c/tests/cpp/eastl/eastl_vector_simple.cpp)

The most direct measurement command is:

```bash
TIMEFORMAT='real %3R user %3U sys %3S'
time ./build-release/c4cll --parse-only \
  -I ./ref/EASTL/include \
  -I ./ref/EABase/include/Common \
  ./tests/cpp/eastl/eastl_vector_simple.cpp >/dev/null
```

Supporting decomposition commands:

```bash
time ./build-release/c4cll --pp-only \
  -I ./ref/EASTL/include \
  -I ./ref/EABase/include/Common \
  ./tests/cpp/eastl/eastl_vector_simple.cpp >/dev/null

time ./build-release/c4cll --lex-only \
  -I ./ref/EASTL/include \
  -I ./ref/EABase/include/Common \
  ./tests/cpp/eastl/eastl_vector_simple.cpp >/dev/null
```

Observed `Release` baseline at the time this idea was written:

- `--pp-only`: about `0.052s`
- `--lex-only`: about `0.082s`
- `--parse-only`: about `12.228s`

This establishes that the problem is parser-dominated rather than
preprocessor- or lexer-dominated.

Additional A/B experiment with the temporary
`ENABLE_HEAVY_TENTATIVE_SNAPSHOT` switch:

- heavy snapshot `ON`: about `11.615s`
- heavy snapshot `OFF`: about `2.415s`

This should be treated as a directional signal, not as a correctness-safe
shipping configuration. It indicates the approximate size of the performance
headroom available if common-path tentative parsing stops doing heavy semantic
state snapshots.

## Acceptance Expectations

The first performance acceptance target should be "clearly measurable
improvement", not theoretical minimum time.

Suggested targets:

- minimum meaningful improvement: `12.2s -> under 9s`
- clear success for the first migration wave: `12.2s -> under 7s`
- strong result: `12.2s -> around 5s`

The `2.4s` experiment should be treated as an approximate lower-bound direction
for future work, not as the initial hard acceptance bar.

Any accepted optimization wave should satisfy all of:

- `Release` parse-only timing improves by at least ~30% on the benchmark file
- parser correctness regressions are not introduced
- the EASTL parse/workflow recipes still pass
