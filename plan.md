# Prealloc CFG Generalization And Authoritative Control-Flow Facts

Status: Active
Source Idea: ideas/open/62_prealloc_cfg_generalization_and_authoritative_control_flow.md
Activated from: ideas/closed/64_shared_text_identity_and_semantic_name_table_refactor.md

## Purpose

Turn idea 62 into an execution runbook that replaces bootstrap-shaped
control-flow recovery with authoritative prepared CFG facts built on the typed
identity layer that idea 64 established.

## Goal

Publish a stable shared-prepare CFG/control-flow contract so downstream
consumers use prepared predecessor, successor, branch-condition, and join
ownership facts instead of re-deriving them from local CFG shape.

## Core Rule

Do not add testcase-shaped branch or join matchers. New work must improve
general CFG ownership and consumer contracts.

## Read First

- `ideas/open/62_prealloc_cfg_generalization_and_authoritative_control_flow.md`
- `ideas/open/63_complete_phi_legalization_and_parallel_copy_resolution.md`
- `ideas/closed/64_shared_text_identity_and_semantic_name_table_refactor.md`
- `src/backend/prealloc/legalize.cpp`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/mir/x86/codegen/`

## Scope

- build authoritative prepared CFG facts for BIR functions
- make join ownership and continuation semantics derive from shared analysis
- move downstream consumers to the shared prepared control-flow contract
- keep symbolic identity on `FunctionNameId` and `BlockLabelId` surfaces

## Non-Goals

- completing phi destruction beyond the CFG facts needed for truthful prepared
  ownership
- generic scalar instruction selection
- unrelated x86 emitter rewrites
- new public CFG contracts keyed by raw strings

## Working Model

- prepare owns the authoritative per-function CFG view
- prepared data publishes predecessor, successor, branch-condition, and join
  ownership facts keyed by semantic ids
- consumers query that prepared contract instead of reconstructing branch and
  join meaning from bootstrap CFG shapes

## Execution Rules

- keep control-flow meaning in shared prepare, not in x86-local reconstruction
- prefer one-time CFG analysis plus typed prepared facts over repeated
  shape-sensitive helper queries
- update `todo.md`, not this file, for routine packet progress
- require `build -> narrow proof` for every accepted code slice
- broaden validation when a slice changes shared prepare plus multiple
  downstream consumer families

## Step 1: Inventory The Existing Prepared Control-Flow Contract

Goal: map the bootstrap control-flow ownership that exists today so the
replacement work stays focused on shared prepared facts instead of ad hoc
consumer rewrites.

Primary targets:

- `src/backend/prealloc/legalize.cpp`
- `src/backend/prealloc/prealloc.hpp`
- current x86 consumer entry points under `src/backend/mir/x86/codegen/`

Actions:

- identify the current prepared branch, continuation, and join facts that
  consumers read
- note the remaining places where consumers still recover meaning from CFG
  shape or local ambiguity heuristics
- confirm the typed `FunctionNameId` and `BlockLabelId` surfaces from idea 64
  are the identity boundary for new work

Completion check:

- the route has a concrete list of authoritative facts to preserve, bootstrap
  heuristics to eliminate, and consumer entry points that must migrate without
  widening scope

## Step 2: Build An Authoritative Shared Prepared CFG Model

Goal: make shared prepare own per-function CFG, branch-condition, and join
  facts as a stable consumer contract.

Primary targets:

- `src/backend/prealloc/legalize.cpp`
- `src/backend/prealloc/prealloc.hpp`
- any extracted shared prepare helpers required to keep the analysis coherent

Actions:

- add or reshape prepared structures so per-block predecessor, successor, and
  branch-condition facts are published on semantic ids
- drive join ownership from authoritative graph analysis rather than ambiguous
  post-hoc recovery
- keep the public prepared surface focused on consumer-ready control-flow
  facts, not on hidden bootstrap shape assumptions

Completion check:

- shared prepare can publish authoritative CFG and join facts for ordinary
  branch ownership without requiring downstream shape reconstruction

## Step 3: Migrate Consumers To The Authoritative Prepared Facts

Goal: move current consumers onto the shared prepared contract so short-circuit
and materialized-select paths stop depending on bespoke CFG interpretation.

Primary targets:

- consumer paths under `src/backend/mir/x86/codegen/`
- shared helpers that currently reconstruct branch or join meaning from CFG
  shape

Actions:

- replace local branch/join reconstruction with reads from the prepared
  control-flow contract
- keep consumer changes narrow to the paths needed to consume the new prepared
  facts
- reject testcase-driven local matcher growth when a shared prepared fact is
  the right fix

Completion check:

- the main consumers read prepared predecessor, successor, branch-condition,
  and join facts directly enough that CFG shape recovery is no longer the
  authoritative path

## Step 4: Validate The CFG Ownership Route

Goal: prove the new shared prepare CFG route without bundling phi-completion or
unrelated backend rewrites.

Actions:

- require a fresh `cmake --build --preset default` for each accepted slice
- use a backend-focused proving subset for routine packets
- broaden validation when a slice changes shared prepare plus multiple
  consumer families or when the route reaches a closure-quality checkpoint

Completion check:

- accepted slices build cleanly and the proving logs show consumer-visible CFG
  ownership behavior is stable on the chosen validation scope
