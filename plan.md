# Parser Error Diagnostics Runbook

Status: Active
Source Idea: ideas/open/parser_error_diagnostics_plan.md
Supersedes: ideas/open/std_vector_bringup_plan.md

## Purpose

Make parser failures easier to localize by instrumenting parser entry points,
capturing parse-function stack state, and exposing a dedicated
`--parser-debug` trace path.

## Goal

Land a consistent `ParseContextGuard`-based tracing model across parser entry
points so default diagnostics stay short while debug mode can expose the parse
function stack behind ambiguous failures.

## Core Rule

Treat this as an observability refactor, not a parser behavior change.

## Read First

- preserve current parse semantics while instrumenting
- prefer parser function names over hand-maintained context labels
- keep default diagnostics compact; full stack output belongs behind
  `--parser-debug`
- do not silently absorb `std::vector` bring-up language fixes into this plan

## Current Target

The active target is parser instrumentation coverage and output shape, starting
from the existing prototype in:

- [`parse.cpp`](/workspaces/c4c/src/frontend/parser/parse.cpp)
- [`parser.hpp`](/workspaces/c4c/src/frontend/parser/parser.hpp)
- [`c4cll.cpp`](/workspaces/c4c/src/apps/c4cll.cpp)

The first validation surface remains:

- `./build/c4cll --parser-debug --parse-only tests/cpp/std/std_vector_simple.cpp`

## Non-Goals

- fixing the underlying `std::vector` parser blockers in this plan
- introducing GCC/Clang-scale human-facing diagnostics
- rewriting every speculative parse helper before instrumentation is in place
- expanding scope beyond parser diagnostics and traceability

## Working Model

1. normalize the parser trace model around parse function names
2. add `ParseContextGuard` to significant `parse_*` entry points
3. keep the best failure snapshot and parse stack through unwinding
4. make `--parser-debug` print the stack while default errors stay short
5. add reduced tests for output shape before deeper speculative parse changes

## Execution Rules

- preserve parser behavior unless a change is required to support diagnostics
- instrument in small slices and verify the compiler still builds after each
  coverage pass
- when adding guards, prefer real parser entry points over tiny helper noise
- if a trace path is too noisy, improve filtering rather than deleting stack data
- record any remaining `std::vector` blocker notes back in its source idea, not
  this active plan

## Ordered Steps

### Step 1: Normalize The Trace Model

Goal:
- settle the parser trace shape around function-stack capture

Actions:
- keep `ParseContextGuard`, `ParseFailure`, and debug-event storage coherent in
  [`parser.hpp`](/workspaces/c4c/src/frontend/parser/parser.hpp)
- ensure the active model records parse function names and stack snapshots
- remove leftover parallel context-label mechanisms if they are no longer needed

Completion Check:
- the parser has one clear internal model for stack-aware diagnostics

### Step 2: Instrument Parser Entry Points

Goal:
- cover the meaningful parser entry surface with guards

Primary Target:
- `parse_*` functions across `expressions.cpp`, `types.cpp`,
  `statements.cpp`, `declarations.cpp`, and `parse.cpp`

Actions:
- add `ParseContextGuard` at the top of significant `parse_*` functions
- avoid over-instrumenting trivial token helpers that do not help traceability
- keep instrumentation consistent across parser modules

Completion Check:
- the important parser families contribute to the internal parse stack

### Step 3: Improve Debug Output Shape

Goal:
- make `--parser-debug` useful for reduction work

Primary Target:
- failure formatting and debug trace emission in
  [`parse.cpp`](/workspaces/c4c/src/frontend/parser/parse.cpp)

Actions:
- keep default diagnostics to one short root-cause line
- print the leaf parse function in normal mode when available
- print the full parse-function stack only in debug mode
- ensure the stack order is stable and easy to read

Completion Check:
- parser debug output clearly shows how control reached the failing parse
  function

### Step 4: Lock In Regression Coverage

Goal:
- protect the diagnostic shape from accidental regression

Actions:
- add one or two reduced parser-only tests for diagnostic output
- validate at least one ambiguous parse family and one expression/declaration
  style failure
- rerun the focused tests plus a compiler build

Completion Check:
- the instrumentation path is covered by reduced tests and the compiler builds

### Step 5: Prepare The Next Diagnostic Slice

Goal:
- leave the codebase ready for committed-failure vs no-match work

Actions:
- identify the first `try_parse_*` family that most needs tri-state failure
  tracking
- record that next target in `plan_todo.md`
- preserve any useful observations about `std::vector` parser blockers back in
  the parked bring-up idea

Completion Check:
- the next speculative-parse diagnostic slice is recorded and bounded
