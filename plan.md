# BIR Aggregate Global Store Handoff Runbook

Status: Active
Source Idea: ideas/open/619_bir_aggregate_global_store_handoff.md

## Purpose

Repair or classify aggregate value stores into global or static storage at the
BIR semantic producer and prepared handoff boundary.

## Goal

Move more than one aggregate/global handoff row beyond the current BIR producer
or prepared handoff stop, or prove that the remaining rows belong to defensible
downstream owners.

## Core Rule

Do not fold aggregate/global handoff work into ordinary local-memory store
production. Preserve the boundary between local aggregate store semantics,
prepared global-data authority, and RV64 consumer lowering.

## Read First

- `ideas/open/619_bir_aggregate_global_store_handoff.md`
- `ideas/open/603_bir_local_memory_store_semantics.md`
- `ideas/open/606_bir_global_initializer_bootstrap.md`
- `ideas/open/608_prepared_global_data_authority.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/high_yield_followup_plan.md`
- Current RV64 gcc torture backend logs under `build/rv64_gcc_c_torture_backend/`
- Current scan summaries under `build/agent_state/`, when present

## Current Targets

Refresh the current aggregate/global handoff set before implementation. Seed
representatives include:

- `src/pr22141-1.c`
- `src/compndlit-1.c`
- `src/pr57344-1.c`
- `src/pr39120.c`
- `src/ieee/20001122-1.c`, only if refreshed diagnostics still show a global
  storage or global-data owner before RV64 terminator consumption
- `src/991030-1.c`, only if refreshed diagnostics still show a global storage
  or global-data owner before RV64 terminator consumption

## Non-Goals

- Do not rework ordinary local-frame scalar or aggregate subobject stores owned
  by idea 603.
- Do not change local-memory GEP, alloca, ABI, runtime, expectation,
  unsupported-marker, allowlist, timeout, or accounting policy.
- Do not implement global initializer bootstrap work owned by idea 606.
- Do not reconstruct missing aggregate/global facts in RV64 codegen when BIR or
  prepared authority is absent.
- Do not edit `ideas/open/619_bir_aggregate_global_store_handoff.md` unless
  source intent itself changes.

## Working Model

Local aggregate subobject stores are already expected to lower for local-only
probes. This plan owns the next boundary: copying or storing aggregate values
into global or static storage. The source value may come from local memory, a
compound literal, a call result, or an array element, but the destination must
be validated as supported global/static storage before RV64 consumer work is
claimed.

## Execution Rules

- Treat refreshed diagnostics as authoritative for the active target set.
- Keep local-only aggregate guard rows in the proof surface so this route does
  not regress idea 603 behavior.
- Prefer semantic BIR or prepared handoff facts over named-case fixes.
- If a row reaches prepared global-data, RV64 consumer, terminator, ABI, or
  runtime ownership, classify it explicitly instead of weakening the diagnostic.
- Code-changing steps need fresh build proof plus focused backend/object
  coverage. The supervisor decides whether broader backend or full validation
  is needed for acceptance.

## Step 1: Refresh Aggregate Global Handoff Evidence

Goal: identify the current aggregate/global handoff rows and the first owner
for each representative.

Actions:

- Inspect current scan summaries and RV64 gcc torture backend logs.
- Reproduce or cite the first stop for seed representatives including
  `src/pr22141-1.c`, `src/compndlit-1.c`, `src/pr57344-1.c`, and
  `src/pr39120.c`.
- Check the split-in candidates `src/ieee/20001122-1.c` and `src/991030-1.c`
  only if current diagnostics still reach global storage or global-data
  ownership before RV64 terminator consumption.
- Record local-only aggregate guard rows that should remain supported.
- Separate BIR producer, prepared handoff, global-data, RV64 consumer, and
  unrelated owners.

Completion check:

- The executor can name the current in-scope row set, cite artifacts proving
  each first owner, and identify guard rows for later proof.
- No implementation changes are required for this step.

## Step 2: Localize The Missing Handoff Authority

Goal: determine whether the fix belongs in BIR semantic production, prepared
handoff publication, or downstream classification.

Actions:

- Trace the aggregate store path from source expression through BIR store or
  copy facts into prepared global/static destination authority.
- Compare local-only aggregate store behavior against global/static handoff
  behavior.
- Identify the exact missing source-value or destination authority for each
  in-scope row.
- Classify rows that already have prepared authority but stop in RV64 consumer
  or global-data layout as downstream owners.

Completion check:

- The active rows are grouped by concrete owner and the implementation target
  for any in-scope repair is narrow enough for a bounded code packet.

## Step 3: Repair BIR Or Prepared Aggregate Global Handoff

Goal: implement the minimal semantic producer or prepared handoff repair needed
for rows owned by this idea.

Actions:

- Add or adjust BIR/prepared facts for aggregate value stores into
  global/static storage without changing local-only store semantics.
- Preserve source-value authority for local memory, compound literal, call
  result, and array-element sources when supported by refreshed evidence.
- Add focused tests around the repaired producer or handoff behavior.
- Run the delegated build and focused backend/object test subset.
- Probe the refreshed representative rows and local-only aggregate guards.

Completion check:

- More than one in-scope aggregate/global handoff row progresses beyond the
  prior BIR/prepared stop or reaches a defensible downstream owner.
- Local-only aggregate guard rows still pass or retain their prior supported
  behavior.
- Focused tests and delegated proof pass.

## Step 4: Classify Residual Owners And Close Readiness

Goal: decide whether idea `619` is complete or needs a narrowed follow-up
runbook.

Actions:

- Refresh targeted probes for the Step 1 row set after the implementation
  packet.
- Confirm no row remains blocked by the same aggregate/global handoff stop
  unless it is outside this idea's authority.
- Classify residual rows by owner: prepared global-data, RV64 consumer,
  terminator, ABI/runtime, global initializer bootstrap, or unresolved.
- Verify no expectation, unsupported-marker, allowlist, timeout, or accounting
  changes were used as progress.

Completion check:

- The plan owner can decide whether idea `619` is close-ready.
- Any remaining work is explicitly assigned to a separate owner instead of
  silently expanding this plan.
