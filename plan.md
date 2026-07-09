# Prepared Dynamic-Frame Callee-Saved Slot Placement Runbook

Status: Active
Source Idea: ideas/open/626_prepared_dynamic_frame_callee_saved_slot_placement.md

## Purpose

Publish explicit prepared callee-saved GPR save-slot placements for dynamic and
fixed frame rows before RV64 object emission consumes those frame plans.

## Goal

Dynamic/fixed frame rows with saved callee-saved GPRs expose concrete save-slot
offset and size facts in prepared frame authority, without RV64 deriving them
from final frame size, register order, source filename, or final assembly shape.

## Core Rule

Prepared frame production owns callee-saved GPR slot placement authority. RV64
consumers may require and validate those prepared facts, but must not infer the
placements locally.

## Read First

- ideas/open/626_prepared_dynamic_frame_callee_saved_slot_placement.md
- docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md
- docs/rv64_gcc_torture_1000_pass_recovery/high_yield_followup_plan.md
- Prepared frame and stack-layout producer code under src/backend/prealloc/
- Prepared frame/object consumer tests under tests/backend/

## Current Targets

- Evidence rows: src/20040811-1.c, src/pr43220.c, src/vla-dealloc-1.c
- Producer target: prepared frame facts for saved callee-saved GPRs
- Consumer target: RV64 object-route diagnostics and guards for missing or
  contradictory prepared placement authority
- Proof surface: prepared dumps, narrow producer/consumer tests, and a broader
  backend subset before close-readiness

## Non-Goals

- Do not implement full RV64 dynamic-frame object lowering in this idea.
- Do not add FPR save/restore placement policy.
- Do not repair move-bundle, local/global memory, runtime, library, variadic,
  pointer-stack-result, or branch-clobber policy.
- Do not change unsupported markers, allowlists, expectations, timeouts, or
  accounting to claim progress.
- Do not add named-case handling for the evidence rows.

## Working Model

- Existing prepared frame facts already identify dynamic stack operation
  metadata, saved callee-saved GPR names, frame size, and alignment.
- The missing authority is a concrete save-slot placement per saved GPR:
  register identity plus stack offset and size, and any existing owner fields
  needed to tie that placement to the prepared frame plan.
- If upstream frame layout does not have enough authority to publish a slot
  placement, the route should keep failing closed with a precise
  missing-authority diagnostic.

## Execution Rules

- Keep source-idea intent stable; record packet progress in todo.md.
- Prefer small producer facts and focused guards over broad frame rewrites.
- Add tests that fail closed when prepared placement authority is absent or
  contradictory.
- Treat testcase-shaped matching, source filename checks, final assembly
  matching, or expectation rewrites as route drift.
- Every code-changing step needs a fresh build plus the delegated narrow proof.

## Step 1: Refresh Callee-Saved Placement Evidence

Goal: Reconfirm the current prepared/object failure shape for the dynamic frame
callee-saved rows.

Actions:
- Build c4cll.
- Generate prepared dumps and RV64 object-route diagnostics for
  src/20040811-1.c, src/pr43220.c, and src/vla-dealloc-1.c.
- Record which prepared frame facts already exist and which callee-saved GPR
  placements are missing.
- Identify the exact diagnostic bucket that should disappear when prepared
  placement authority is published.

Completion check:
- todo.md records the evidence rows, current prepared facts, missing placement
  facts, residual owner buckets, and proof command output path.

## Step 2: Locate Producer Authority

Goal: Identify the narrow producer location that already has enough frame-layout
authority to publish callee-saved GPR slot placements.

Actions:
- Inspect prepared frame/stack-layout construction in src/backend/prealloc/.
- Trace saved callee-saved GPR metadata from frame layout into prepared frame
  facts.
- Identify or design the narrow helper that maps a saved GPR to its concrete
  prepared slot placement.
- Confirm where rows without sufficient authority should be rejected rather
  than inferred.

Completion check:
- todo.md names the producer function/helper targets, existing carrier fields
  or needed narrow carrier addition, and the fail-closed condition for missing
  authority.

## Step 3: Publish Prepared Save-Slot Placement Facts

Goal: Add producer-side prepared facts for callee-saved GPR save-slot offsets
and sizes.

Actions:
- Extend the prepared frame carrier only as narrowly as needed for saved GPR
  slot placements.
- Populate the placement facts from producer-owned frame-layout authority.
- Preserve existing dynamic stack operation metadata, fixed-slot frame-pointer
  metadata, frame size, and frame alignment.
- Add or update producer-level tests that prove representative rows publish
  concrete placements before RV64 object emission.

Completion check:
- Build passes.
- Focused producer tests pass.
- Prepared dumps for at least one representative row expose concrete
  callee-saved GPR save-slot placement offsets and sizes.

## Step 4: Guard RV64 Consumer Use Of Prepared Placement Authority

Goal: Make the RV64 object route require producer-published callee-saved GPR
slot placements without implementing broad dynamic-frame lowering.

Actions:
- Update RV64 prepared frame consumption to read the producer-published
  placement facts where the route reaches callee-saved save/restore handling.
- Keep missing or contradictory placement authority fail-closed.
- Add guard tests for missing placement, mismatched register identity, and
  contradictory offset or size.
- Do not infer placement from final frame size, register order, source filename,
  or final assembly text.

Completion check:
- Build passes.
- Focused RV64 object-emission guard tests pass.
- Diagnostics prove the consumer depends on prepared placement authority and
  rejects incomplete authority.

## Step 5: Breadth And Close-Readiness

Goal: Prove idea 626's source-authority bucket is addressed and classify any
remaining failures outside this idea.

Actions:
- Re-run the Step 1 evidence rows through prepared dumps and RV64 diagnostics.
- Run the supervisor-delegated broader backend subset.
- Confirm the original missing callee-saved placement authority bucket is gone
  or replaced only by out-of-scope residual owners.
- Record residuals by owner instead of extending this route into adjacent
  initiatives.

Completion check:
- Broader backend subset passes.
- todo.md records close-readiness evidence, remaining residual owner buckets,
  and whether idea 626 should close or split.
