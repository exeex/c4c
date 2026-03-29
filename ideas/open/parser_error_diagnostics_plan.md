# Parser Error Diagnostics Improvement Plan

## Goal

Improve parser diagnostics across `src/frontend/parser/*.cpp` so syntax failures
are easier for humans and LLM agents to localize, classify, and reduce.

The target is **not** GCC-style maximal, verbose diagnostics.

The target is:

- short, stable, machine-traceable error messages
- parser-side function-stack capture for parse entry paths
- clear distinction between speculative parse failure and committed syntax error
- enough grammar context to understand which parse function / branch failed
- recovery behavior that preserves the most useful failure site instead of
  collapsing into late "unexpected token" fallout


## Why This Matters

The parser is increasingly using `try_parse_*` helpers to probe ambiguous C/C++
grammar.

That makes the parser more capable, but it also changes the diagnostic problem:

- a `false` result can mean "this grammar branch does not apply here"
- or it can mean "this branch partially matched and then failed"

Those two cases are semantically very different, but the current code often
treats them the same.

For parser bring-up and header compatibility work, that loses valuable
information. The user or agent sees only the final flattened failure, not the
path that got there.


## Current Problems

### 1. `try_parse_*` often collapses "no match" and "syntax error" into the same result

Many speculative helpers return only `bool`.

That is not enough to distinguish:

- `NoMatch`: branch was not applicable, keep trying alternatives
- `CommittedFailure`: branch looked applicable, consumed meaningful structure,
  then hit malformed syntax
- `Success`

Examples in the current parser:

- template argument probes in `types.cpp`
- scoped type / declarator probes in `types.cpp`
- record-member dispatch helpers in `types.cpp`

This causes useful branch-local failure information to disappear.

### 2. Exceptions are frequently swallowed during speculative parse attempts

Some `try_parse_*` paths use `catch (...) { pos_ = saved_pos; return false; }`.

That restores the token cursor, but it also discards:

- what branch was being tried
- what token caused failure
- whether the parser had already committed to a branch
- which expectation actually failed

This is especially harmful in ambiguous grammar zones such as:

- template type-vs-value argument parsing
- record member dispatch
- declarator parsing with parenthesized / function-pointer forms

### 3. Final diagnostics are too flat

The current top-level reporting shape is mostly:

- `expected X but got Y`
- `unexpected token 'Z'`

Those messages are compact, which is good, but they are missing branch context.

For traceability, we usually also need:

- which grammar family failed
- whether the parser was probing or already committed
- whether recovery skipped ahead

Without that, a late token error may point to fallout rather than root cause.

### 4. Recovery can hide the best failure location

Top-level and record-body recovery intentionally skip to `;` or `}`.

That is good for continuing the parse, but right now the parser does not
reliably preserve the best pre-recovery failure context before skipping.

This creates cases where the visible error is:

- later than the real syntax break
- less specific than the original expectation
- detached from the grammar branch that failed

### 5. Diagnostic state is too minimal for structured tracing

`parser.hpp` currently tracks coarse parser health such as:

- `had_error_`
- `parse_error_count_`

What it does not track yet:

- the furthest/most-interesting failure position
- the current parse context stack
- branch-local speculative failure info
- whether a failure happened before or after branch commitment

That limits both human debugging and future tooling.


## Desired Diagnostic Model

Diagnostics should stay short, but become more structured.

The current preferred direction is:

- record parse function entry with `ParseContextGuard`
- preserve the active parse-function stack internally
- on failure, snapshot the best stack and failing token
- print only the most useful leaf parse function in normal mode
- print the full parse stack only in debug mode

A good target shape is one primary line plus optional parser-debug notes.

For example:

```text
error: parse_fn=parse_declarator_parameter_list phase=committed expected=RPAREN got='&&'
```

and with parser debug enabled:

```text
note: parser debug trace follows
[pdebug] kind=enter fn=parse_param line=67 col=15
[pdebug] kind=leave fn=parse_param line=67 col=24
[pdebug] stack: -> parse_stmt -> parse_expr -> parse_param
```

Properties we want:

- stable field names
- one root cause per emitted diagnostic
- no giant prose blobs
- easy to grep, diff, and feed back into an agent loop
- debug output separated cleanly from default diagnostics


## Proposed Improvements

### Phase 1: Instrument parse entry points with `ParseContextGuard`

Add lightweight parser state for function-stack-aware diagnostics, and then
apply it consistently to parser entry points.

Current implementation direction:

- `ParseContextGuard`
- `ParseContextFrame`
- `ParseFailure`
- `ParseDebugEvent`

Near-term objective:

- every significant `parse_*` function should push a guard on entry
- the parser should maintain a stack of active parse function names
- failures should snapshot the best active stack before unwinding

This should live in `parser.hpp` / `parse.cpp` and be generic enough for all
parser modules.

### Phase 2: Replace `bool`-only speculative parsing in the highest-value paths

Prioritize ambiguous areas that currently matter most:

1. template argument parsing
2. qualified type / declarator parsing
3. record member dispatch

Upgrade those helpers from:

- raw `bool`

to something that can represent:

- no match
- committed failure
- success

This is the core semantic improvement.

### Phase 3: Preserve parse-function stack when rewinding

When a speculative parse rewinds the token cursor, keep the failure metadata if
the branch had already committed.

That means:

- rewinding token position should not automatically erase the diagnostic trail
- only true `NoMatch` paths should disappear silently

### Phase 4: Make emitted diagnostics slightly structured

Keep the output concise, but normalize it around a few fields:

- `parse_fn`
- `expected`
- `got`
- `line`
- `col`
- optional `recovery`

Avoid large paragraph diagnostics unless there is no structured alternative.

### Phase 5: Add focused regression tests for diagnostic quality

Add reduced parser-only cases that assert both:

- the parse fails in the intended place
- the emitted diagnostic contains the expected context markers
- the parser-debug path shows the expected parse-function stack shape

Candidate test areas:

- malformed template argument lists
- malformed operator/member declarations in records
- broken parenthesized declarators
- malformed dependent type names


## Non-Goals

Do not:

- try to fully replicate Clang/GCC diagnostic richness
- turn the parser into a maximal-recovery compiler frontend in one step
- block ongoing `std::vector` bring-up on a broad diagnostic refactor
- rewrite every parser helper at once


## Current Experimental Status

An initial instrumentation slice has already started on the working branch:

- `c4cll` has a new `--parser-debug` flag
- parser-side debug/failure structs were added in `parser.hpp`
- `ParseContextGuard` now records parse function names rather than a separate
  hand-written grammar label
- a small set of parse entry points already carry guards
- current debug output proves the mechanism works, but coverage is still sparse
  and speculative parse rewinds still hide some root causes

This confirmed two design choices:

- keeping only parse function names is simpler and preferable to maintaining a
  parallel hand-written context taxonomy
- the full parse-function stack should be preserved internally even if normal
  diagnostics only print the leaf function


## Suggested First Active Slice

The best next implementation slice is:

1. add `ParseContextGuard` to all significant `parse_*` functions
2. normalize the parser-debug output around parse function names and full stack
   snapshots
3. keep the default diagnostic short, printing only the leaf parse function
4. add one or two reduced tests that validate the message shape and debug path

This keeps the work narrow, high-value, and directly useful for later
speculative-parse diagnostics work.


## Success Criteria

This idea is successful when:

- malformed parser input reports one short root-cause diagnostic with branch
  context
- all meaningful `parse_*` entry points contribute to the internal stack trace
- `--parser-debug` can show the parse-function stack that led to the failure
- speculative parse rewinds no longer erase useful committed failures
- reduced tests cover at least one ambiguous `try_parse_*` family
- parser diagnostics become materially easier for an LLM agent to use for
  iterative reduction and fix planning
