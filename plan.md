# Pointer-Loaded-From-Global Local-Memory Policy Runbook

Status: Active
Source Idea: ideas/open/639_pointer_loaded_from_global_local_memory_policy.md

## Purpose

Define the RV64 authority boundary for local-memory accesses whose selected
address is a pointer SSA value loaded from a global object.

## Goal

Move at least one complete-authority pointer-loaded-from-global local-memory
row past its current owner, or reclassify the family to a more precise owner
with current evidence.

## Core Rule

Do not treat `bir.load_global ptr @x` followed by local-memory use as direct
`addr @x` local-memory support. The loaded pointer value needs explicit
identity, freshness, extent, offset, address-space, and local-memory-use
authority before RV64 can consume it.

## Read First

- `ideas/open/639_pointer_loaded_from_global_local_memory_policy.md`
- `ideas/closed/631_direct_global_symbol_local_memory_policy.md`
- `ideas/open/621_rv64_prepared_global_value_location_consumer.md`
- `ideas/open/633_aggregate_stack_home_local_memory_policy.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`

## Current Targets

- Representative rows named by the source idea: `src/pr46309.c`,
  `src/pr58984.c`, and `src/pr66556.c`
- Proof surface: a pointer SSA value loaded from a global object, then consumed
  as a local-memory address
- Current family boundary: pointer-loaded-from-global local memory, not direct
  global-symbol local memory, prepared global value-location consumption,
  aggregate stack homes, or ordinary frame slots

## Non-Goals

- Do not reopen direct global-symbol local-memory support from idea 631.
- Do not absorb prepared global value-location work from idea 621.
- Do not absorb aggregate, sret, byval, or stack-home policy from idea 633.
- Do not claim progress from expectations, unsupported markers, allowlists,
  timeouts, pass/fail accounting, runtime policy, or diagnostic wording alone.
- Do not add source-file-specific handling for the representative rows.

## Working Model

The candidate rows load a pointer value from a global object before the
local-memory access. The implementation route must prove where that pointer
identity and freshness are published, where its extent/offset/address-space
facts live, and which component is allowed to select it for local-memory use.

## Execution Rules

- Start every packet from refreshed diagnostics; do not assume the Step 5
  classification from idea 631 is still the first owner.
- Keep reclassification precise. If a row is really a prepared global
  value-location, aggregate home, string constant, direct global-symbol, or
  runtime-only row, route it out instead of widening this policy.
- Prefer semantic producer or RV64 consumer rules over testcase-shaped
  matching.
- Preserve fail-closed diagnostics for missing pointer freshness, missing
  global source identity, incomplete extent or width facts, unsupported address
  space, and ambiguous memory-use authority.
- Update `todo.md` with packet progress and proof; rewrite this runbook only
  if the route or step boundaries are wrong.

## Ordered Steps

### Step 1: Refresh and classify pointer-loaded-from-global rows

Goal: Confirm the current residual family and first owner for each candidate
row.

Primary target: `src/pr46309.c`, `src/pr58984.c`, `src/pr66556.c`

Actions:

- Run the supervisor-selected build and narrow proof command for the
  representative rows.
- Capture the BIR/HIR/RV64 diagnostics around `bir.load_global ptr @...` and
  the later `bir.load_local ... addr %...` local-memory consumer.
- Record producer value, pointer SSA value, global source object, freshness,
  selected offset, width, extent, address space, and current rejection reason.
- Separate rows that are actually direct global-symbol, prepared value-location,
  aggregate home, string-constant, ordinary frame-slot, or runtime-only cases.

Completion check:

- `todo.md` lists each refreshed row with its current first owner and exact
  policy bucket, and at least one row remains a genuine
  pointer-loaded-from-global local-memory candidate or all rows are precisely
  reclassified.

### Step 2: Locate the missing authority boundary

Goal: Decide whether the first repair belongs in a producer fact, an RV64
consumer rule, or a reclassification route.

Primary target: the shared row family from Step 1

Actions:

- Trace where pointer identity, freshness, global source identity, extent,
  offset, width, and address-space facts are produced or lost.
- Inspect whether the local-memory consumer has an explicit selected-use
  authority for the loaded pointer value.
- Compare the shape against prepared global value-location and aggregate
  stack-home policies before choosing an owner.
- Identify one shared complete-authority shape for implementation, or record
  the exact missing fact that prevents a legal first packet.

Completion check:

- The packet names one concrete producer or RV64 consumer boundary to repair,
  with negative cases that must stay fail-closed.

### Step 3: Implement one semantic authority packet

Goal: Advance one complete-authority pointer-loaded-from-global local-memory
shape without admitting unsupported neighbors.

Primary target: the boundary selected in Step 2

Actions:

- Add the narrow producer or RV64 consumer support needed for the selected
  authority shape.
- Consume only explicit pointer identity, freshness, extent, offset,
  address-space, and local-memory-use facts.
- Add or update focused tests that prove one legal shape and one missing or
  mismatched authority rejection.
- Avoid expectation rewrites or unsupported-marker changes as capability
  evidence.

Completion check:

- A complete-authority representative advances past the current first owner,
  and nearby missing-authority cases remain rejected with precise diagnostics.

### Step 4: Validate and decide next lifecycle action

Goal: Prove the implemented packet did not broaden unrelated local-memory
policy and decide whether idea 639 is complete or needs another packet.

Primary target: Step 3 diff, representative rows, and adjacent policy buckets

Actions:

- Run the supervisor-selected build, narrow proof, and any broader regression
  subset required by the touched layer.
- Recheck negative proof for direct global-symbol rows, prepared
  value-location rows, aggregate homes, string constants, and
  missing-freshness pointer values.
- Record any residual rows with precise first owners in `todo.md`.
- If the source idea is satisfied, report readiness for plan-owner closure; if
  not, leave the next semantic packet explicit.

Completion check:

- The supervisor has fresh proof for the accepted authority packet and enough
  lifecycle evidence to continue, split, or close the idea without guessing.
