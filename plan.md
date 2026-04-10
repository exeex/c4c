# Parser Symbol Tables And Speculative State Compaction

Status: Active
Source Idea: ideas/open/03_parser_symbol_tables_and_speculative_state_compaction.md

## Purpose

Reduce parser-side speculative parsing overhead by isolating mutable parser
symbol/state tables behind parser-local helper types and narrower access points.

## Goal

Make tentative parsing depend on stable parser-state operations instead of
directly reaching into multiple top-level STL containers on `Parser`.

## Core Rule

Preserve parser correctness and rollback behavior first; keep this work parser
local and behavior preserving unless a correctness fix is required.

## Read First

- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/parser_core.cpp`
- parser implementation files that mutate typedef, variable-type, namespace, or
  alias lookup state during tentative parsing
- `prompts/EXECUTE_PLAN.md`

## Scope

- group parser symbol/state tables into named parser-local structs instead of
  many unrelated top-level `Parser` members
- distinguish string-keyed lookup tables from denser parser bookkeeping where
  it helps clarify ownership
- introduce parser-local wrappers or accessors so future storage changes do not
  require touching every call site
- keep the slice small enough to validate with parser-focused tests

## Non-Goals

- no sema or HIR storage redesign
- no backend work
- no repo-wide container migration
- no parser grammar changes unless required for correctness
- no journaled rollback model in this runbook

## Working Model

- `Parser` should expose a smaller set of parser-state operations
- lookup storage should live under explicit parser-local state objects
- tentative parsing code should rely on those state objects instead of ad hoc
  direct member access
- dense-ID or rollback-friendly reshaping is preparatory only in this runbook

## Execution Rules

- prefer small, test-backed slices over broad container rewrites
- preserve helper names and call patterns when they already express intent
- when a new wrapper is introduced, migrate the narrowest coherent set of call
  sites with it
- keep rollback-sensitive behavior observable and easy to compare before/after

## Ordered Steps

### Step 1: Inventory parser state tables and speculative touch points

Goal: identify which parser members act as mutable symbol/state tables and which
tentative parsing paths read or write them.

Primary targets:

- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/parser_core.cpp`
- nearby parser implementation files discovered by search

Actions:

- list current parser members that hold typedef, variable-type, namespace-name,
  and `using` alias state
- identify the narrowest cohesive subset with speculative parsing relevance
- note helper functions that can become wrappers instead of broad direct access

Completion check:

- the first implementation slice has a clearly bounded ownership surface

### Step 2: Introduce parser-local state-table grouping

Goal: replace the chosen top-level parser members with one or more named
parser-local structs that preserve current behavior.

Primary targets:

- `src/frontend/parser/parser.hpp`

Actions:

- add grouped parser-state structs for the chosen lookup tables
- keep names explicit and local to parser concerns
- preserve existing semantics and initialization behavior

Completion check:

- parser builds with grouped state storage and no behavior change

### Step 3: Migrate a narrow speculative parsing slice to wrappers

Goal: move the chosen speculative call sites onto parser-local accessors or
state references instead of direct top-level member access.

Primary targets:

- `src/frontend/parser/parser_core.cpp`
- parser files that own the chosen state mutations

Actions:

- introduce the smallest useful wrapper/accessor layer
- update only the coherent call sites needed for the current slice
- keep rollback-sensitive operations easy to trace

Completion check:

- direct access is reduced for the targeted slice without widening scope

### Step 4: Validate parser behavior and record follow-on work

Goal: prove the refactor did not regress parser correctness and capture the next
slice cleanly.

Actions:

- add or update narrow parser-focused tests if needed
- run targeted parser validation first, then broader regression checks as
  required by `prompts/EXECUTE_PLAN.md`
- record remaining direct-access surfaces or deferred storage changes in
  `todo.md`

Completion check:

- targeted validation passes and the next parser-state slice is explicitly
  recorded
