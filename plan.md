# Parser State Convergence And Scope Rationalization

Status: Active
Source Idea: ideas/open/81_parser_state_convergence_and_scope_rationalization.md

## Purpose

Make parser-owned mutable state easier to read and reason about before later
semantic-scope and `TextId` cleanup.

## Goal

Regroup parser state into named bundles with explicit lifetime boundaries so
future parser changes can distinguish semantic scope from namespace, pragma,
debug, and tentative rollback state.

## Core Rule

Do not broaden this into a general parser rewrite or a repo-wide identity
migration; keep every change inside the parser subsystem and preserve behavior
unless a structural move forces a correctness fix.

## Read First

- ideas/open/81_parser_state_convergence_and_scope_rationalization.md
- src/frontend/parser/parser.hpp
- src/frontend/parser/parser_state.hpp
- parser implementation files that manipulate parser-owned mutable state

## Scope

- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/parser_state.hpp`
- parser implementation files that directly depend on parser-owned mutable
  state layout

## Non-Goals

- no immediate replacement of every parser string path with `TextId`
- no backend or HIR work
- no grammar changes unless required by a structural move
- no testcase-shaped narrowing of the route

## Working Model

- treat parser state layout as the primary seam, not the lexical grammar itself
- separate semantic lexical scope from namespace, template, pragma, debug, and
  tentative rollback mechanisms
- keep parser entry surface in `parser.hpp` and move state-bearing support types
  to `parser_state.hpp`
- use the regrouped state layout to expose clear ownership before any later
  shared-helper or `TextId` follow-up

## Execution Rules

- prefer small behavior-preserving moves over mixed refactors
- keep `parser.hpp` focused on API and method index, not snapshot or bundle
  internals
- do not promote execution churn back into the idea file unless durable intent
  changes
- after each structural move, validate with
  `cmake --build build -j --target c4c_frontend c4cll`
- run focused parser/frontend tests that cover tentative parsing and
  scope-heavy paths before broadening
- escalate to broader `ctest` only when a step crosses multiple parser
  subsystems
- use `todo.md` for packet state and route checkpoints; rewrite `plan.md` only
  when the active contract changes

## Validation

- `cmake --build build -j --target c4c_frontend c4cll`
- focused parser/frontend tests covering tentative parsing and scope-heavy
  paths
- broader `ctest` only if a step crosses multiple parser subsystems

## Step 1: Consolidate State-Bearing Structs Into `parser_state.hpp`

Goal: move parser state support types out of `parser.hpp` without changing
parser behavior.

Primary targets:

- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/parser_state.hpp`
- parser implementation files that still define or include state support types

Actions:

- identify state-bearing structs, snapshots, and other support types still
  embedded in `parser.hpp`
- move types that are not part of the true parser entry surface into
  `parser_state.hpp`
- keep `parser.hpp` limited to parser API, method declarations, and
  entry-facing declarations
- preserve include dependencies so existing parser implementation files
  continue to compile

Completion check:

- `parser_state.hpp` owns the extracted state support types and `parser.hpp`
  no longer carries them

## Step 2: Regroup Parser Member Fields Into Explicit Bundles

Goal: make the parser's mutable ownership boundaries readable by clustering
fields into named bundles.

Primary targets:

- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/parser_state.hpp`
- parser implementation files that initialize or snapshot parser-owned state

Actions:

- regroup related members into explicit bundles such as input, bindings,
  record/enum definitions, namespace, template, pragma, diagnostics, and
  tentative rollback
- keep behavior unchanged while moving fields
- ensure rollback and push/pop helpers still restore the same semantic surface

Completion check:

- parser member layout matches explicit ownership bundles and no field lost its
  lifecycle coverage

## Step 3: Classify Push/Pop And Rollback Paths By Kind

Goal: separate real semantic scope from other parser context machines.

Primary targets:

- parser implementation files that implement push/pop, save/restore, or
  tentative parse guards

Actions:

- classify each mechanism as semantic lexical scope, namespace context,
  template scope, pragma stack, debug context, or tentative rollback
- note where helpers can be shared and where the mechanism must stay distinct
- confirm the classification explains current lifetime boundaries without
  changing grammar behavior

Completion check:

- the parser's state transitions are documented and grouped by mechanism
  instead of being treated as one flat stack

## Step 4: Define The Next Follow-On Slice For Scope Convergence

Goal: turn the regrouped parser state layout into the next bounded execution
slice.

Primary targets:

- `todo.md`
- parser files identified by the prior step as the next narrow follow-on

Actions:

- record the next narrow slice needed to continue semantic scope convergence
- keep later `TextId` cleanup explicitly out of this runbook until the state
  layout is stable
- document any cross-subsystem dependency that now needs its own idea file
  instead of expanding this one

Completion check:

- the active runbook ends with a concrete next slice and a clear boundary for
  later `TextId` work
