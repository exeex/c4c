# BIR Prealloc Call ABI Authority Boundary Audit Runbook

Status: Active
Source Idea: ideas/open/97_bir_prealloc_call_abi_authority_boundary_audit.md

## Purpose

Audit the boundary between BIR call ABI facts and prealloc call-plan/runtime
helper inference before x86 and RISC-V backend rebuild work starts.

## Goal

Classify call ABI authority overlaps between BIR and prealloc, then create
follow-up ideas only for concrete duplicated authority or unclear contracts.

## Core Rule

This plan is audit-only. Do not edit implementation files, change target ABI
behavior, rewrite expectations, or claim backend capability progress from
classification alone.

## Read First

- `ideas/open/97_bir_prealloc_call_abi_authority_boundary_audit.md`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/lir_to_bir/calling.cpp`
- `src/backend/bir/lir_to_bir/call_abi.cpp`
- `src/backend/bir/lir_to_bir/module.cpp`
- `src/backend/prealloc/legalize.cpp`
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/calls.hpp`
- `src/backend/prealloc/variadic_entry_plans.cpp`
- `src/backend/prealloc/i128_runtime_helpers.cpp`
- `src/backend/prealloc/f128_runtime_helpers.cpp`
- `src/backend/prealloc/regalloc/call_moves.cpp`

## Scope

- BIR-produced call ABI, result ABI, calling convention, direct callee, and
  typed call metadata facts.
- Prealloc call-plan, wrapper, result, variadic, i128, f128, special-carrier,
  and call-move inference/classification.
- Follow-up idea creation only when an overlap is concrete, owned, and
  proofable.

## Non-Goals

- Do not implement call ABI cleanup in this plan.
- Do not change call-plan layout, runtime-helper behavior, target ABI behavior,
  or test expectations.
- Do not move target-sensitive ABI legalization into BIR.
- Do not reopen AArch64 calls cleanup routes.
- Do not treat all target-sensitive ABI legalization as duplicated BIR
  authority.

## Working Model

Classify each traced overlap with one or more of:

- `intentional-prealloc-legalization`
- `bir-fact-consumed-correctly`
- `prealloc-rederives-bir-fact`
- `bir-missing-target-neutral-fact`
- `contract-ambiguous`
- `needs-follow-up-idea`

Use `todo.md` for packet notes, traced functions, classifications, and proof
results. Only create or change source ideas when the audit produces durable
follow-up intent.

## Execution Rules

- Keep every classification tied to concrete BIR producers, prealloc consumers,
  and the specific fact being reconstructed or consumed.
- Separate target-neutral call semantics from target-sensitive ABI
  legalization.
- Treat target ABI legalization, runtime helper planning, and physical
  call-move preparation as prealloc authority unless the trace proves
  re-derivation of an existing BIR fact.
- Create follow-up ideas only for narrow boundaries with named files and proof
  routes.
- If implementation files are accidentally touched, stop and require fresh
  build or compile proof before accepting the slice.

## Steps

### Step 1: Inventory BIR Call ABI Facts

Goal: identify the call-related facts BIR already produces and name their
intended authority.

Primary targets:

- `src/backend/bir/bir.hpp`
- `src/backend/bir/lir_to_bir/calling.cpp`
- `src/backend/bir/lir_to_bir/call_abi.cpp`
- `src/backend/bir/lir_to_bir/module.cpp`

Actions:

- Inspect BIR call, function, argument, result, callee, calling-convention, and
  typed metadata structures.
- Trace where those facts are created from LIR.
- Record each fact in `todo.md` with its producer and target-neutral authority
  boundary.

Completion check:

- `todo.md` lists the BIR call ABI fact inventory and identifies which facts
  need prealloc-consumer tracing.

### Step 2: Inventory Prealloc Call ABI Inference

Goal: identify where prealloc classifies, legalizes, or reconstructs call ABI
facts while building prepared call routes.

Primary targets:

- `src/backend/prealloc/legalize.cpp`
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/calls.hpp`
- `src/backend/prealloc/regalloc/call_moves.cpp`

Actions:

- Trace call-plan construction, argument/result classification, wrapper
  selection, and call-move planning.
- Separate target-sensitive legalization from target-neutral semantic fact
  reconstruction.
- Record each prealloc fact source or classifier in `todo.md`.

Completion check:

- `todo.md` names the prealloc call ABI inference sites and their initial
  authority classification.

### Step 3: Trace Runtime Helper And Special Carrier Plans

Goal: classify runtime-helper, variadic, i128, f128, and special-carrier call
planning against the BIR/prealloc authority boundary.

Primary targets:

- `src/backend/prealloc/variadic_entry_plans.cpp`
- `src/backend/prealloc/i128_runtime_helpers.cpp`
- `src/backend/prealloc/f128_runtime_helpers.cpp`
- related definitions in `src/backend/prealloc/calls.hpp`

Actions:

- Trace helper call and variadic entry plan inputs back to BIR facts or target
  profile/legalization inputs.
- Identify any duplicated direct callee, argument ABI, result ABI, or typed
  metadata derivation.
- Record intentional target-sensitive legalization separately from contract
  ambiguity.

Completion check:

- `todo.md` classifies runtime-helper, variadic, i128, f128, and
  special-carrier call planning as retained, ambiguous, missing BIR fact, or
  follow-up material.

### Step 4: Cross-Map Overlaps And Contract Gaps

Goal: decide which BIR/prealloc overlaps are correct consumption, duplicated
authority, missing BIR facts, or ambiguous contracts.

Primary targets:

- the Step 1 BIR fact inventory in `todo.md`
- the Step 2 and Step 3 prealloc inference inventories in `todo.md`
- the source files already traced by earlier steps

Actions:

- Cross-map every BIR call ABI fact to its prealloc consumers.
- For each overlap, classify it with the working-model labels.
- Name the exact producer, consumer, and fact for any
  `prealloc-rederives-bir-fact`, `bir-missing-target-neutral-fact`, or
  `contract-ambiguous` classification.

Completion check:

- `todo.md` contains a complete overlap table and clearly separates
  intentional legalization from duplicated or ambiguous authority.

### Step 5: Synthesize Follow-Up Ideas Or Intentional Retention

Goal: finish the audit with durable conclusions and create follow-up ideas
only where the trace proves a narrow, proofable boundary.

Primary targets:

- `todo.md`
- `ideas/open/` for any generated follow-up ideas
- `ideas/open/97_bir_prealloc_call_abi_authority_boundary_audit.md` only if
  closure or durable source-intent notes are required

Actions:

- Summarize each call ABI overlap and final classification.
- For every `needs-follow-up-idea` classification, create a focused source
  idea with concrete owned files, proof expectations, and reviewer reject
  signals.
- Explicitly list overlaps judged `intentional-prealloc-legalization` and
  `bir-fact-consumed-correctly`.
- Prepare the close decision for the source idea, including generated follow-up
  idea names or an explicit statement that none were generated.

Completion check:

- The audit has classified all scoped call ABI overlaps, generated only
  supported follow-up ideas, and left the active source idea ready for
  lifecycle close review.
