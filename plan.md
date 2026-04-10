# Tentative Parse Guard Tiering And Snapshot Reduction

Status: Active
Source Idea: ideas/open/52_tentative_parse_guard_tiering_and_snapshot_reduction.md

## Purpose

Reduce parser-side speculative parsing cost by moving common disambiguation
paths off the current whole-state snapshot model without weakening rollback
safety for correctness-sensitive parses.

## Goal

Introduce a lite tentative parse guard, keep the heavy guard for semantic
rollback cases, migrate the safest hot-path probes first, and validate both
correctness and parser-only timing on EASTL-heavy inputs.

## Core Rule

Prefer behavior-preserving parser performance work. Keep speculative parsing
debuggable, keep heavy rollback available, and do not broaden this slice into a
general parser architecture rewrite.

## Read First

- [ideas/open/52_tentative_parse_guard_tiering_and_snapshot_reduction.md](/workspaces/c4c/ideas/open/52_tentative_parse_guard_tiering_and_snapshot_reduction.md)
- [src/frontend/parser/parser.hpp](/workspaces/c4c/src/frontend/parser/parser.hpp)
- [src/frontend/parser/parser_support.cpp](/workspaces/c4c/src/frontend/parser/parser_support.cpp)
- [src/frontend/parser/parser_expressions.cpp](/workspaces/c4c/src/frontend/parser/parser_expressions.cpp)
- [src/frontend/parser/parser_statements.cpp](/workspaces/c4c/src/frontend/parser/parser_statements.cpp)
- [src/frontend/parser/parser_declarations.cpp](/workspaces/c4c/src/frontend/parser/parser_declarations.cpp)
- [src/frontend/parser/parser_types_declarator.cpp](/workspaces/c4c/src/frontend/parser/parser_types_declarator.cpp)
- [src/frontend/parser/parser_types_struct.cpp](/workspaces/c4c/src/frontend/parser/parser_types_struct.cpp)

## Current Targets

- parser tentative parsing guard design and call-site classification
- lightweight guard usage instrumentation
- safe migration of common-path disambiguation probes
- parser regression coverage for ambiguity cases
- Release timing checks on EASTL parser-heavy input

## Non-Goals

- removing heavy rollback support entirely
- changing parser semantics only in Release builds
- replacing all speculative parsing with token rewind in one pass
- hiding a broad parser rewrite inside this performance slice
- expanding into unrelated parser cleanups unless they directly block this plan

## Working Model

- `TentativeParseGuard` remains the correctness-first heavy rollback path.
- A new lite guard rewinds only cheap parser state and token mutation state.
- Call sites must be tiered explicitly rather than migrated opportunistically.
- Instrumentation should show heavy-vs-lite usage so follow-on work is measured.

## Execution Rules

- Keep source-idea scope intact; do not fold unrelated parser work into this
  runbook.
- Use the smallest testable migration slice at each call site.
- Add or update focused parser tests before changing a guarded ambiguity path.
- Re-measure parser-only timing after each meaningful migration wave.
- If a call site proves semantically unsafe for lite rollback, move it back to
  the heavy path and record the reason in `todo.md`.

## Step 1: Inspect Existing Tentative Guard State

Goal: map current heavy snapshot contents, rollback behavior, and tentative
guard call sites before changing behavior.

Primary targets:

- [src/frontend/parser/parser.hpp](/workspaces/c4c/src/frontend/parser/parser.hpp)
- [src/frontend/parser/parser_support.cpp](/workspaces/c4c/src/frontend/parser/parser_support.cpp)
- parser files that instantiate `TentativeParseGuard`

Actions:

- identify the exact `ParserSnapshot` fields and restore logic
- inventory tentative parsing call sites by file and ambiguity kind
- separate obviously semantic probes from likely syntax-only probes
- note any token mutation or nesting state that a lite guard must preserve

Completion check:

- the active call-site inventory and safety notes are recorded in `todo.md`

## Step 2: Add Lite Guard And Shared Instrumentation

Goal: introduce a lightweight speculative parsing path with explicit metrics
without changing call-site behavior yet.

Primary targets:

- [src/frontend/parser/parser.hpp](/workspaces/c4c/src/frontend/parser/parser.hpp)
- [src/frontend/parser/parser_support.cpp](/workspaces/c4c/src/frontend/parser/parser_support.cpp)

Actions:

- define a lightweight parser snapshot structure for cheap rewind state only
- add `TentativeParseGuardLite` with explicit commit and rollback behavior
- preserve the existing heavy guard API and semantics
- add counters or summary diagnostics for guard entry, commit, rollback, and
  heavy-vs-lite usage

Completion check:

- the tree builds and guard instrumentation can distinguish heavy and lite use
  without changing parser correctness

## Step 3: Migrate Safest Common-Path Probes

Goal: move the lowest-risk disambiguation sites onto the lite guard first.

Primary targets:

- [src/frontend/parser/parser_expressions.cpp](/workspaces/c4c/src/frontend/parser/parser_expressions.cpp)
- [src/frontend/parser/parser_statements.cpp](/workspaces/c4c/src/frontend/parser/parser_statements.cpp)
- [src/frontend/parser/parser_declarations.cpp](/workspaces/c4c/src/frontend/parser/parser_declarations.cpp)

Actions:

- start with `sizeof(type)` vs `sizeof(expr)` probing if the inventory confirms
  it is syntax-only
- migrate `if` declaration-condition probing where semantic rollback is not
  required
- migrate range-for and constructor-style init vs function-declaration
  lookahead only after targeted tests exist
- leave template arg type-vs-value parsing and other semantic probes on the
  heavy path unless safety is demonstrated

Completion check:

- selected call sites use the lite guard, focused ambiguity tests pass, and no
  heavy-only semantic site is downgraded without proof

## Step 4: Validate Correctness

Goal: prove the tiered guard model preserves parser behavior on focused
regressions and existing coverage.

Primary targets:

- parser ambiguity regression tests
- EASTL parser workflows

Actions:

- add or update tests for:
  - `sizeof(type)` vs `sizeof(expr)`
  - range-for parsing
  - `if` declaration conditions
  - template arg type-vs-value parsing
  - constructor-style init vs function declaration ambiguity
- run targeted parser tests after each migration slice
- run broader regression coverage before handoff
- spot check with heavy snapshot modes `ON` and `OFF` when relevant

Completion check:

- targeted regressions and broader parser coverage stay green with the selected
  lite migrations enabled

## Step 5: Measure Performance And Decide Follow-On Work

Goal: confirm the slice delivers material parser-only improvement and decide
whether journaled rollback is still needed.

Primary targets:

- [tests/cpp/eastl/eastl_vector_simple.cpp](/workspaces/c4c/tests/cpp/eastl/eastl_vector_simple.cpp)

Actions:

- capture Release timing for `--pp-only`, `--lex-only`, and `--parse-only`
- compare parser-only time against the baseline recorded in the source idea
- inspect instrumentation output to confirm heavy guard usage decreased on the
  common path
- record whether the remaining cost justifies a later journaled rollback idea

Completion check:

- timing results and the next-slice recommendation are recorded in `todo.md`
