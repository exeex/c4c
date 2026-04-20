# Prepared CFG Authoritative Completion And Parallel-Copy Correctness

Status: Active
Source Idea: ideas/open/64_prepared_cfg_authoritative_completion_and_parallel_copy_correctness.md
Activated from: ideas/closed/59_generic_scalar_instruction_selection_for_x86.md

## Purpose

Turn idea 64 into an execution runbook that finishes authoritative prepared
CFG publication and parallel-copy correctness without drifting back into
heuristic CFG recovery or testcase-shaped fixes.

## Goal

Make prepared control-flow the sole authoritative source for covered backend
consumers by completing control-flow publication, sourcing join ownership from
prepared CFG facts, and carrying cyclic parallel-copy temporary-save semantics
through move resolution.

## Core Rule

Do not patch one failing CFG or phi shape at a time. Repair the missing
authoritative model and the missing end-to-end cycle-correct transfer
semantics instead of growing more fallback heuristics around the old route.

## Read First

- `ideas/open/64_prepared_cfg_authoritative_completion_and_parallel_copy_correctness.md`
- `src/backend/prealloc/legalize.cpp`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/regalloc.cpp`

## Scope

- complete prepared control-flow publication when authoritative block/edge
  metadata exists
- move residual join ownership off raw BIR CFG rediscovery and onto prepared
  control-flow facts
- carry `SaveDestinationToTemp` or equivalent temporary-save semantics through
  the consumer-facing move-resolution path
- validate the route with build plus backend-focused proof, broadening only
  when shared backend surfaces warrant it

## Non-Goals

- target-specific emitter rewrites unrelated to authoritative CFG publication
- reopening string-backed prepared identity
- testcase-shaped select or short-circuit matcher growth as a substitute for
  general prepared CFG ownership

## Working Model

- `PreparedControlFlowFunction` should exist whenever authoritative prepared
  block or edge facts exist for a function
- join ownership belongs to prepared CFG publication, not to downstream raw-BIR
  shape recovery
- cyclic parallel-copy bundles are only correct when their temporary-save
  semantics survive into the actual move-resolution consumer
- downstream consumers should either read authoritative prepared CFG data or
  stay unsupported; they should not silently fall back

## Execution Rules

- prefer one bounded authoritative-CFG seam per packet over broad prealloc
  rewrites
- keep correctness ownership upstream in shared prepare; do not hide missing
  semantics in x86-local recovery
- update `todo.md`, not this file, for routine packet progress
- require `build -> ^backend_` proof for accepted code slices unless the
  delegated packet explicitly justifies another subset
- broaden validation when a slice changes shared prealloc contracts or multiple
  backend buckets move together

## Step 1: Audit Authoritative Publication Gaps

Goal: map the exact places where prepared CFG publication still drops required
authoritative facts before making behavior changes.

Primary targets:

- `src/backend/prealloc/legalize.cpp`
- `src/backend/prealloc/prealloc.hpp`

Actions:

- inspect how `PreparedControlFlowFunction` records are created and identify
  the conditions that still omit them when only block or edge metadata exists
- trace where join ownership is still rediscovered from raw BIR CFG shape
  instead of read from prepared control-flow data
- confirm how cyclic `PreparedParallelCopyBundle` steps are represented today
  and where temporary-save semantics stop short of consumer-visible resolution

Completion check:

- the remaining publication, join-ownership, and parallel-copy gaps are each
  tied to concrete producer or consumer seams in the primary targets

## Step 2: Complete Prepared Control-Flow Publication

Goal: ensure authoritative prepared CFG functions and join facts are published
whenever covered block or edge metadata exists.

Primary targets:

- `src/backend/prealloc/legalize.cpp`
- `src/backend/prealloc/prealloc.hpp`

Actions:

- update prepared control-flow construction so functions with authoritative
  block or edge metadata still publish `PreparedControlFlowFunction`
- move remaining join ownership to prepared CFG facts instead of raw-BIR
  rediscovery
- keep the packet bounded to shared prepare ownership; do not widen into
  target-specific codegen cleanup

Completion check:

- covered consumers can source function/block/join control-flow facts from the
  prepared model without silent omission or raw-BIR fallback

## Step 3: Carry Parallel-Copy Temp Semantics Through Resolution

Goal: make cyclic parallel-copy plans correct at the consumer-facing move
resolution layer.

Primary targets:

- `src/backend/prealloc/legalize.cpp`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/regalloc.cpp`

Actions:

- trace `SaveDestinationToTemp` or equivalent steps from prepared planning into
  the move-resolution consumer
- add the missing consumer-facing contract so cyclic bundles preserve the
  required temporary-save ordering
- keep the route general and semantic; do not special-case one CFG or phi
  testcase shape

Completion check:

- cyclic prepared parallel-copy bundles resolve through an explicit temporary-
  save contract rather than a plan-only marker

## Step 4: Validate Authoritative CFG Consumption

Goal: prove the authoritative CFG route with backend-focused coverage that
exercises the repaired producer and consumer surfaces.

Actions:

- require a fresh build for every accepted slice
- use `^backend_` as the default proof bucket for this prealloc/backend route
- broaden only when shared backend surfaces move enough that backend-only proof
  stops being credible
- reject slices whose practical effect is heuristic fallback growth rather than
  authoritative CFG repair

Completion check:

- accepted slices have fresh proof logs and validation proportional to the
  authoritative-CFG blast radius
