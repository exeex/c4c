# Parser Debug Surface For Vector/STL Work

Status: Active
Source Idea: ideas/open/parser_debug_surface_for_vector_stl_work.md

## Purpose

Improve parser-debug observability so STL-style parser bring-up can continue
without ad hoc instrumentation or another refactor pass.

## Goal

Make parser-debug answer, with opt-in high-signal output, which token window
was active, which tentative branch committed or rolled back, whether injected
token streams were involved, and at least one narrower CLI debug mode.

## Core Rule

Prefer small, behavior-preserving debug additions that directly help
`std::vector` / `eastl::vector` parser work. Do not redesign the parser or turn
this into a broad tracing rewrite.

## Read First

- `ideas/open/parser_debug_surface_for_vector_stl_work.md`
- parser debug CLI entrypoints and option parsing
- parser best-failure reporting and debug event emission
- parser sites that do speculative parsing or temporary token-stream swaps
- motivating tests:
  - `tests/cpp/std/std_vector_simple.cpp`
  - `tests/cpp/eastl/eastl_vector_simple.cpp`

## Current Targets

- best-failure output should expose local token context
- tentative parse lifecycle should be visible when explicitly enabled
- injected token-stream parsing should be visible when explicitly enabled
- CLI should provide at least one narrower parser-debug surface beyond the
  current all-or-nothing mode
- validation should cover motivating STL-style cases and focused parser-debug
  behavior

## Non-Goals

- do not redesign parser tracing infrastructure
- do not broaden scope into unrelated STL parsing fixes unless they block this
  work
- do not make default parse errors noisy
- do not overfit the output to a single testcase

## Working Model

Implement this in narrow slices:

1. expose the highest-signal failure-local context first
2. add tentative/injected instrumentation behind targeted debug controls
3. add one or more narrower CLI views only after the payload is useful
4. validate on real STL-style cases and focused parser-debug checks

## Execution Rules

- keep richer diagnostics opt-in
- preserve current successful parse behavior unless the step explicitly adds a
  success-path summary mode
- add or update focused tests before each feature slice
- use Clang/LLVM only as behavior reference when a testcase or output shape
  needs comparison
- if a separate parser feature gap appears, record it as a new idea instead of
  expanding this runbook silently

## Ordered Steps

### Step 1: Land Failure-Local Token Context

Goal: make parse failures show enough nearby token detail to localize nested
template failures quickly.

Primary targets:

- parser best-failure summary/reporting
- parser debug data structures if they need token metadata

Actions:

- inspect how best-failure state is stored and emitted today
- add token index, token kind, lexeme/spelling, and a small surrounding token
  window to the failure-local output
- keep the default presentation concise enough for normal parser failure use
- add or update focused tests that assert the new context is emitted

Completion check:

- a targeted parse failure exposes token-local context sufficient to inspect the
  neighborhood around the failing token

### Step 2: Add Tentative Parse Lifecycle Visibility

Goal: surface which speculative parse branches enter, commit, or roll back.

Primary targets:

- speculative/tentative parse helpers
- parser debug event definitions and emission logic
- parser-debug CLI gating for tentative events

Actions:

- identify the parser helpers that save/restore position for speculation
- add `tentative_enter`, `tentative_commit`, and `tentative_rollback` events
- include start/end positions and a short detail where practical
- ensure this extra trace is emitted only when the relevant debug mode is
  enabled
- add focused tests for at least one ambiguity case

Completion check:

- a targeted debug run can show tentative branch lifecycle without flooding the
  default output

### Step 3: Mark Injected Token-Stream Parsing

Goal: make token-stream swaps or injected parses explicit in the debug trace.

Primary targets:

- parser sites that temporarily replace or inject `tokens_`
- parser debug event emission around those hot spots

Actions:

- inspect known token-swap and injected-parse call sites
- add begin/end markers for injected or swapped token streams
- preserve original behavior and restoration semantics
- add focused coverage for at least one injected-token site

Completion check:

- a targeted debug run can distinguish original-token parsing from injected or
  swapped token-stream work

### Step 4: Add Narrower CLI Debug Surfaces

Goal: provide at least one higher-signal CLI entrypoint beyond `--parser-debug`.

Primary targets:

- command-line option parsing
- parser debug formatting/reporting path

Actions:

- choose the smallest useful surface from the idea, with bias toward summary,
  token-window, tentative, injected, filter, or limit controls
- wire the new flag(s) into parser debug configuration
- keep `--parser-debug` as the simple catch-all switch
- add CLI-focused tests that lock the intended behavior

Completion check:

- the CLI offers at least one narrower parser-debug mode that is useful on the
  motivating STL-style files

### Step 5: Validate On Motivating STL Cases

Goal: prove the new observability is useful on the stated bring-up targets.

Primary targets:

- `tests/cpp/std/std_vector_simple.cpp`
- `tests/cpp/eastl/eastl_vector_simple.cpp`
- nearby parser-debug tests

Actions:

- run targeted parse-only and parser-debug commands on both motivating files
- verify success-path or failure-path output stays readable and actionable
- run nearby parser/debug tests and the broader regression suite expected by the
  active slice
- record any out-of-scope follow-on ideas back into `ideas/open/` if discovered

Completion check:

- the new parser-debug surface materially reduces the need for ad hoc
  instrumentation during STL-style parser work

## Done Condition

This runbook is complete when:

- parser-debug includes useful failure-local token context
- tentative commit/rollback behavior is observable when requested
- injected-token parsing is visible when requested
- the CLI exposes at least one narrower parser-debug mode
- motivating STL-style cases can be investigated without another debug
  observability refactor
