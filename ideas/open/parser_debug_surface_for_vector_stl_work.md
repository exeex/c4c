# Parser Debug Surface For Vector/STL Work

Status: Open
Last Updated: 2026-04-02

## Goal

Strengthen parser diagnostics, parser-debug trace output, and CLI inspection
surfaces so that STL-like parse bring-up can continue without repeatedly
stopping for internal refactors.

Immediate motivating cases:

- `tests/cpp/std/std_vector_simple.cpp`
- `tests/cpp/eastl/eastl_vector_simple.cpp`


## Why This Idea Exists

Recent parser cleanup made the codebase much easier to navigate, but the next
push is no longer structural cleanup for its own sake. The next push is feature
pressure from real C++ library shapes such as `std::vector` and `eastl::vector`.

Those cases are valuable because they stress:

- nested template-id parsing
- qualified/dependent type lookup
- member typedef / alias lookup
- constructor/member-call syntax in templated contexts
- allocator-heavy signatures and long declarator chains

The current parser debug system is already useful, but when a failure occurs in
these cases it still tends to be missing the most actionable context:

- the exact token window near the failure
- whether a tentative parse branch committed or rolled back
- whether parser logic temporarily swapped/injected token streams
- a more targeted CLI interface for parse-debug-only investigation


## Current Useful Surface

Already present:

- `--parse-only`
- `--parser-debug`
- best-failure summary (`parse_fn=...`, expected/got, detail)
- debug event stream with enter/leave/fail tracking

This is enough to keep moving, but not yet ideal for STL-scale parser work.


## Concrete Current UX On `std_vector_simple.cpp`

Current motivating file:

- `tests/cpp/std/std_vector_simple.cpp`

### Current commands

Plain parse-only check:

```bash
./build/c4cll --parse-only tests/cpp/std/std_vector_simple.cpp
```

Current parser-debug invocation:

```bash
./build/c4cll --parser-debug --parse-only tests/cpp/std/std_vector_simple.cpp
```

### What happens today

For the current `std_vector_simple.cpp`, parse currently succeeds, so:

- `--parse-only` produces no useful inspection output on success
- `--parser-debug --parse-only` also produces little or no output on success
- there is no "summary of what parser-debug observed" mode for successful runs
- there is no token-window or tentative-branch summary emitted proactively

In other words, parser-debug today is mostly failure-driven:

- if parsing succeeds, there is almost nothing to inspect
- if parsing fails, the parser can print a best-failure summary and a debug
  event stream

### Current failure-style output shape

When parser-debug does fire on a parse failure, the output shape is roughly:

```text
<file>:<line>:<col>: error: parse_fn=<fn> phase=committed expected=<...> got='<...>' detail="..."
<file>:<line>:<col>: note: parser debug trace follows
[pdebug] kind=enter fn=<fn> line=<line> col=<col>
[pdebug] kind=soft_fail fn=<fn> line=<line> col=<col> detail="..."
[pdebug] kind=leave fn=<fn> line=<line> col=<col>
[pdebug] stack: -> parse_top_level -> ...
```

Useful current properties:

- best-failure function summary
- committed vs soft-fail distinction
- compact stack summary
- ordered event stream

Still missing from the current output:

- token index at the failure site
- `TokenKind` at the failure site
- surrounding token window
- explicit tentative branch commit/rollback lifecycle
- explicit token-stream injection/swap markers
- a success-path parser-debug summary

### Why this is a problem for `std::vector` bring-up

`std::vector`-style files often do not fail immediately at the syntactic point
that explains the bug. A parser may recover, backtrack, or choose the wrong
tentative branch before the visible error appears.

For this class of failure, "best failure + event stream" is helpful but still
too indirect. We want to be able to answer:

1. what exact token neighborhood was active at the interesting point?
2. which speculative branch won?
3. did a token-injected parse path get involved?
4. if parse succeeded, what suspicious near-miss branches happened anyway?

### Proposed additional CLI / inspection modes

Keep the current simple flags:

- `--parse-only`
- `--parser-debug`

Add more specific interfaces:

```bash
./build/c4cll --parser-debug-summary --parse-only tests/cpp/std/std_vector_simple.cpp
./build/c4cll --parser-debug-tokens --parse-only tests/cpp/std/std_vector_simple.cpp
./build/c4cll --parser-debug-tentative --parse-only tests/cpp/std/std_vector_simple.cpp
./build/c4cll --parser-debug-injected --parse-only tests/cpp/std/std_vector_simple.cpp
./build/c4cll --parser-debug-filter=parse_template_argument_list --parse-only tests/cpp/std/std_vector_simple.cpp
./build/c4cll --parser-debug-limit=400 --parse-only tests/cpp/std/std_vector_simple.cpp
```

Possible structured mode:

```bash
./build/c4cll --parser-debug-json --parse-only tests/cpp/std/std_vector_simple.cpp
```

### What we want those commands to show

#### `--parser-debug-summary`

Even on success, print a short parser-debug summary such as:

```text
parser_debug: success
focus=parse_template_argument_list
deepest_stack=parse_top_level -> parse_local_decl -> parse_type_name -> parse_template_argument_list
tentative_rollbacks=3
token_window=[Identifier 'vector'] [Less '<'] [KwInt 'int'] [Greater '>']
```

#### `--parser-debug-tokens`

When a failure or focus point is selected, print token-local context:

```text
token_index=123 kind=Greater lexeme='>'
window:
  [120] Identifier 'vector'
  [121] Less '<'
  [122] KwInt 'int'
  [123] Greater '>'
  [124] Identifier 'a'
  [125] Assign '='
```

#### `--parser-debug-tentative`

Show speculative branch lifecycle:

```text
[pdebug] kind=tentative_enter fn=try_parse_template_type_arg start=121
[pdebug] kind=tentative_rollback fn=try_parse_template_type_arg start=121 end=122 detail="not followed by comma or template close"
[pdebug] kind=tentative_enter fn=try_parse_template_non_type_arg start=121
[pdebug] kind=tentative_commit fn=try_parse_template_non_type_arg start=121 end=123
```

#### `--parser-debug-injected`

Show token-stream swaps/injected parses explicitly:

```text
[pdebug] kind=injected_parse_begin fn=parse_base_type detail="template-instantiated base type probe"
[pdebug] kind=injected_parse_end fn=parse_base_type detail="restored original token stream"
```

### Short-term implementation bias

For the current `std_vector_simple.cpp` case, the most useful next step is not
"more of the same full trace". It is:

1. success-path summary mode
2. failure-local token windows
3. tentative parse lifecycle visibility
4. injected-token markers

Those four improvements would make `std::vector` / `eastl::vector` parser work
materially easier without needing a full parser debugger redesign.


## Main Objective

Make parser-debug output answer these questions directly:

1. Which token window did the parser actually fail on?
2. Which tentative branch committed, and which one rolled back?
3. Was the parser operating on the original token stream or an injected one?
4. Can a user request narrower, higher-signal parser diagnostics from the CLI?


## Priority Enhancements

### 1. Failure-local token context

Add failure output that includes:

- `token_index`
- current `TokenKind`
- current token spelling
- a small surrounding token window (for example `[-3, +5]`)

Reason:

For nested template cases, knowing only `got='>'` or `got='vector'` is often
not enough. We need to see the neighboring `::`, `<`, `>`, `>>`, identifiers,
and delimiters.


### 2. Tentative parse lifecycle tracing

Add optional trace events for tentative parse branches:

- `tentative_enter`
- `tentative_commit`
- `tentative_rollback`

Each event should ideally include:

- starting `pos_`
- ending `pos_`
- owning parse function
- optional short reason/detail

Reason:

The parser now relies heavily on speculative parsing for type-vs-value and
declarator disambiguation. STL code makes those branches hotter and deeper.


### 3. Token-stream swap / injection markers

Emit explicit debug markers when parser code temporarily replaces `tokens_`
with an injected token stream.

Candidate event names:

- `token_stream_swap_begin`
- `token_stream_swap_end`
- `injected_parse_begin`
- `injected_parse_end`

Reason:

Without this, debug output can mislead readers into thinking every failure
occurred on the original file token stream.


### 4. More targeted CLI flags

Keep `--parser-debug`, but consider adding narrower switches such as:

- `--parser-debug-tokens`
  - print token windows around best failure
- `--parser-debug-tentative`
  - include tentative branch lifecycle events
- `--parser-debug-injected`
  - include token-stream swap/injection events
- `--parser-debug-limit=N`
  - control max stored debug events
- `--parser-debug-filter=<fn>`
  - only emit/retain events involving one parse function family

Reason:

`--parser-debug` should remain the simple "turn it on" switch, but STL bring-up
often benefits from narrower, less noisy views.


### 5. Parse-debug-oriented test harness coverage

Add or extend small internal tests that assert parser-debug output remains
useful for known stress shapes.

Targets:

- template argument type-vs-value ambiguity
- nested `std::vector<...>` / `eastl::vector<...>` style cases
- injected-token parse sites
- parenthesized cast vs expression ambiguity


## Nice-To-Have Interfaces

These are useful, but not required before returning to feature work:

### 1. Parse debug summary mode

A CLI mode that prints only:

- best failure summary
- compact stack
- failure token window

without the full event stream.

Possible flag:

- `--parser-debug-summary`


### 2. Structured debug output

Optional machine-readable parser-debug output:

- JSON lines
- single JSON blob

Possible flag:

- `--parser-debug-json`

This could help future tooling/agents compare failures automatically.

Recommended first shape:

- `--parser-debug-dump=json`

with output that includes at least:

- parser invocation metadata
- best-failure summary
- full debug event list
- token metadata per event (`token_index`, `TokenKind`, `lexeme`, location)
- stack snapshots or enough information to reconstruct them
- tentative branch ids and lifecycle markers
- injected-token / token-stream swap markers


### 3. Full dump + drill-down workflow

The intended workflow should be two-stage:

1. emit one complete parser debug dump
2. query that dump for narrower views

Example:

```bash
./build/c4cll --parser-debug-dump=json --parse-only tests/cpp/std/std_vector_simple.cpp > parser_dump.json
```

Then inspect with narrower queries such as:

- by parse function
- by token index
- by source location
- by tentative branch id
- by event kind
- by best-failure chain

This is preferable to forcing every investigation through one giant text trace.


### 4. Query / drill-down interfaces

Possible follow-up CLI:

```bash
./build/c4cll --parser-debug-query parser_dump.json --best-failure
./build/c4cll --parser-debug-query parser_dump.json --token-index 123
./build/c4cll --parser-debug-query parser_dump.json --loc 12:21
./build/c4cll --parser-debug-query parser_dump.json --fn parse_next_template_argument
./build/c4cll --parser-debug-query parser_dump.json --branch 17
./build/c4cll --parser-debug-query parser_dump.json --kinds fail,soft_fail,tentative_rollback
```

High-value query behaviors:

- `--best-failure`
  - print committed failure, related soft failures, compact stack, token window
- `--token-index N`
  - print local token window, surrounding events, active stack, branch state
- `--loc L:C`
  - source-oriented form of token lookup
- `--fn NAME`
  - filter events/branches touching one parse helper family
- `--branch N`
  - print tentative branch lifecycle from enter to commit/rollback
- `--kinds ...`
  - filter to interesting event classes only

The key idea is:

- `dump` captures all facts
- `query` turns those facts into focused diagnostics


### 5. Agent-facing skill / tooling path

JSON output opens the door to a dedicated skill for parser investigation.

Possible future skill responsibilities:

- run parser with full debug dump enabled
- summarize best failure and likely hot parse families
- answer targeted questions from the dump
- extract token windows and tentative branch histories
- compare two dumps before/after a parser patch

This would let an agent investigate parser regressions without adding ad hoc
instrumentation or manually scanning a very long raw trace every time.

Possible future skill shape:

- skill name similar to `parser-debug-inspector`
- input: testcase path plus optional focus (`fn`, `token`, `branch`, `kind`)
- output: concise diagnosis plus references into the full JSON dump

This is not required for the first CLI patch, but the dump format should be
designed so a skill can consume it later without needing another format rewrite.


### 6. Function-family presets

Preset debug focuses for common investigation areas:

- `template`
- `qualified-type`
- `declarator`
- `record-body`

Possible flag:

- `--parser-debug-preset=template`


## Non-Goals

- do not redesign the parser around a new tracing framework
- do not block STL parse work on perfect debug tooling
- do not turn parser debug into a full debugger before adding direct value
- do not overfit output only to one failing testcase


## Execution Plan

### 1. Add highest-signal failure-local context

Implement:

- token index
- token kind
- token window in best-failure output

This should be the first patch because it improves almost every parse failure.


### 2. Instrument speculative parsing

Add tentative branch trace events with minimal noise.

Keep default output readable; only emit the extra lifecycle events when the
relevant parser-debug mode is enabled.


### 3. Mark injected-token parsing

Touch the known manual token-swap hot spots and add explicit trace markers.

Hot spots already known in parser code:

- injected base-type parsing
- injected template/member lookup parsing
- cast/template edge cases with manual token save/restore


### 4. Add CLI slicing controls and dump/query surfaces

Implement narrower parser-debug switches only after the debug payload itself is
useful.

Preferred order:

1. full JSON dump
2. summary mode
3. token-window support
4. tentative branch ids/events
5. query/drill-down interface
6. optional presets / skill integration


### 5. Validate on real motivating cases

Use both:

- `tests/cpp/std/std_vector_simple.cpp`
- `tests/cpp/eastl/eastl_vector_simple.cpp`

Success means:

- parser failures become easier to localize when they happen
- no need to add ad hoc `fprintf` while pushing STL parse support


## Acceptance Criteria

This idea is complete when:

1. parser-debug output includes enough local token context to diagnose nested
   template failures
2. speculative parse rollback/commit behavior can be observed when needed
3. injected-token parsing is visible in the trace when enabled
4. the CLI offers at least one narrower parser-debug interface beyond the
   current all-or-nothing output
5. STL-style parser bring-up can continue without pausing for another debug
   observability refactor


## Notes To Future Agents

- Prefer small, high-signal debug additions over broad tracing churn.
- Optimize for the next library bring-up task, not an abstract perfect debugger.
- Keep default parse errors concise; richer detail should remain opt-in.
