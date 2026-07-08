# Prepared Global Data Authority Runbook

Status: Active
Source Idea: ideas/open/608_prepared_global_data_authority.md
Activated from: ideas/open/608_prepared_global_data_authority.md

## Purpose

Complete prepared/global authority for supported global object data, prepared
global memory facts, and direct global-symbol base-plus-offset addressing
before any later RV64 consumer route relies on those facts.

## Goal

Move multiple prepared/global authority rows past their current producer-side
stops while RV64/global emission, access lowering, link behavior, runtime
behavior, and unsupported policy remain outside this plan.

## Core Rule

Publish only prepared/global facts that the producer layer can prove. Do not
repair prepared authority by reconstructing object bytes, relocation targets,
memory extents, or base-plus-offset identity inside RV64 target code.

## Read First

- ideas/open/608_prepared_global_data_authority.md
- docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md
- docs/rv64_gcc_torture_1000_pass_recovery/high_yield_followup_plan.md
- docs/rv64_gcc_torture_1000_pass_recovery/dependency_order_to_1000.md
- src/backend/prealloc/prepared_contract_verifier.cpp
- src/backend/prealloc/object_data.hpp

## Current Targets

- Prepared global memory fact rows, including representative rows such as
  `src/strlen-7.c`.
- Direct global-symbol base-plus-offset authority rows, including
  representative rows such as `src/pr79737-2.c`.
- Any remaining selected global object-data contract rows that still fail in
  prepared/global authority after the mixed object-data slot route and RV64
  global consumer route have moved their owned work.

## Non-Goals

- RV64 global symbol emission, relocation-record emission, or global
  load/store access-width lowering.
- BIR global initializer bootstrap, local-memory producer repairs, ABI,
  runtime/link behavior, expectations, unsupported markers, allowlists,
  timeouts, or accounting.
- Testcase-shaped fixes for a single representative row.
- Helper-only refactors that leave the same prepared/global authority stop.

## Working Model

The global-data bucket is split by first owner. Prepared/global authority must
publish coherent facts before RV64 consumes globals. Prior lifecycle work moved
relocation-only and mixed object-data slot authority far enough for the RV64
consumer route to run, and idea `609` has now closed its consumer side. This
runbook reopens the producer side of idea `608` and starts by refreshing which
authority rows still belong here.

## Execution Rules

- Start with an inventory packet before editing implementation code.
- Keep selected object-data, prepared global memory facts, and direct
  global-symbol base-plus-offset authority as separate packets unless the
  inventory proves one shared producer helper owns them.
- Preserve fail-closed diagnostics for missing initializer facts, unsupported
  sections, ambiguous object identity, unknown extents, unsupported widths, and
  unresolved direct base-plus-offset authority.
- Do not change tests, expectations, unsupported markers, external allowlists,
  timeout/accounting files, or RV64 target code as a substitute for prepared
  authority.
- Prove code slices with a focused build and the supervisor-selected backend
  or RV64 gcc-torture subset. Escalate to broader validation when a slice
  changes shared prepared-contract verification or object-data publication.

## Ordered Steps

### Step 1: Refresh prepared/global authority inventory

Goal: identify the current residual rows whose first owner is still
prepared/global authority after the closed mixed object-data and RV64 consumer
work.

Primary targets:
- `build/rv64_gcc_c_torture_backend/<case-id>/case.log`
- prepared dumps or diagnostics for global memory facts and direct
  global-symbol base-plus-offset addressing
- `src/backend/prealloc/prepared_contract_verifier.cpp`
- `src/backend/prealloc/object_data.hpp`

Actions:
- Build or refresh an allowlist of prepared/global authority representatives
  from the selected object-data, prepared global memory fact, and direct
  global-symbol base-plus-offset families.
- Record each row's current first diagnostic and the producer facts it lacks.
- Separate rows now owned by RV64 consumer, link/runtime, ABI, or unsupported
  policy from rows still owned by prepared/global authority.
- Select the first implementation packet from the largest confirmed
  prepared/global authority family.

Completion check:
- `todo.md` records the selected allowlist, residual owner split, first
  implementation family, and exact proof command for Step 2.

### Step 2: Publish supported prepared global memory facts

Goal: produce supported prepared global memory facts when BIR/global
initializer or symbol facts already prove object identity, extent, and access
requirements.

Primary targets:
- prepared/prealloc global-memory fact publication code identified in Step 1
- `src/backend/prealloc/prepared_contract_verifier.cpp`
- focused prepared or backend tests that already cover global memory facts

Actions:
- Follow existing prepared-fact structures instead of inventing a target-local
  global model.
- Publish facts only for supported object identity, offset, width, and extent
  combinations.
- Preserve diagnostics for missing initializer facts, unsupported access
  widths, ambiguous identity, unknown extents, and non-global storage.
- Keep RV64 load/store lowering out of this packet.

Completion check:
- Multiple prepared global memory fact rows progress beyond the prior prepared
  authority stop, while unsupported-width and missing-authority rows remain
  accurately rejected.

### Step 3: Publish direct global-symbol base-plus-offset authority

Goal: represent direct global-symbol base-plus-offset memory addressing in the
prepared layer when source identity and offset are authoritative.

Primary targets:
- prepared/prealloc memory operand publication code identified in Step 1
- prepared contract verification for direct global-symbol base-plus-offset
  facts
- focused tests or RV64 gcc-torture rows such as `src/pr79737-2.c`

Actions:
- Publish direct global-symbol base-plus-offset facts from proven producer
  inputs only.
- Preserve fail-closed behavior for ambiguous symbols, missing base identity,
  unsupported offsets, unknown extents, and rows that require BIR producer
  repair first.
- Do not lower RV64 addressing or infer symbol identity from final object
  emission.

Completion check:
- Multiple direct global-symbol base-plus-offset rows move to the next
  downstream owner or pass the prepared authority stop, with missing-authority
  guard rows still rejected.

### Step 4: Recheck selected object-data authority residuals

Goal: confirm whether selected global object-data contract work remains in
idea `608` after prior relocation-only, mixed-slot, and RV64 consumer work.

Primary targets:
- selected object-data publication and verification helpers
- object-data prepared dumps from the Step 1 allowlist
- guard rows from the prior mixed object-data and RV64 consumer routes

Actions:
- Re-run the selected object-data residuals from Step 1.
- If residuals still fail in prepared/global authority, repair only the
  missing producer-side fact representation.
- If residuals now belong to RV64, link/runtime, ABI, or unsupported policy,
  record that owner split in `todo.md` instead of changing implementation.
- Keep relocation-record emission and global symbol emission out of this plan.

Completion check:
- Remaining selected object-data authority rows are either repaired at the
  prepared layer or documented as no longer owned by idea `608`.

### Step 5: Prove prepared/global handoff and close readiness

Goal: validate the prepared/global authority boundary and record whether the
source idea is ready for lifecycle close review.

Primary targets:
- supervisor-selected prepared/global authority allowlist
- focused prepared/backend tests touched by the implementation packets
- `todo.md`

Actions:
- Re-run the focused build and selected proof subset after implementation.
- Record rows that moved through prepared/global authority and their next
  owner.
- Record residual rows that still belong to BIR initializer bootstrap,
  RV64/global consumer, link/runtime, ABI, unsupported policy, or a new
  lifecycle idea.
- Return to the supervisor for review, broader validation, or lifecycle
  routing.

Completion check:
- `todo.md` contains the movement summary, residual owner split, proof
  command, and whether `ideas/open/608_prepared_global_data_authority.md` is
  ready for close review or should remain open with a narrower follow-up.
