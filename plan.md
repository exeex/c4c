# Semantic Result Frame-Slot Materialization Point Producer Plan

Status: Active
Source Idea: ideas/open/481_semantic_result_frame_slot_materialization_point_producer.md
Resumed After: ideas/closed/482_semantic_frame_slot_materialization_probe_decomposition.md

## Purpose

Resume semantic result frame-slot materialization-point producer work from the
focused decomposition seam instead of auditing the monolithic `%t23` route
again.

## Goal

Produce explicit prepared materialization-point facts only when a semantic
instruction result has durable producer evidence that it was materialized into
a specific frame slot.

## Core Rule

Use the focused scalar compare frame-slot destination probe as the first input
seam. Semantic identity plus destination/layout is necessary evidence, but it
is not materialization authority by itself.

## Read First

- ideas/open/481_semantic_result_frame_slot_materialization_point_producer.md
- ideas/closed/482_semantic_frame_slot_materialization_probe_decomposition.md
- build/agent_state/482_step4_scalar_compare_destination_probe/decision.md
- build/agent_state/482_step3_probe_seam_bindings/bindings.md
- tests/backend/case/riscv64_scalar_compare_frame_slot_destination.c

## Current Target

- Focused positive input seam:
  - `%t2 = bir.ne i32 %t0, %t1`
  - `bir.store_local %lv.comparison, i32 %t2`
  - stack home for `%t2`
  - destination frame-slot layout for `%lv.comparison`
  - store-site frame-slot access row
  - `store_source ... source_producer=binary`
- Required next decision:
  - prove a sound materialization-point producer packet from explicit evidence;
  - or route the exact missing lower-level producer without returning to the
    monolithic `930930-1` route.

## Non-Goals

- RV64 branch-load emission or target lowering.
- Populating downstream semantic intervals, frame-slot source facts, or
  `PreparedBranchStackLoadAuthority` rows.
- Treating storage-only movement such as `%t22 -> %t23` as semantic
  materialization when authority is absent.
- Solving select-result stack-destination publication, pointer-value
  provenance, or unsupported terminator relationships.
- Inferring materialization points from raw BIR adjacency, final homes, storage,
  offsets, object ids, value names, function names, testcase names, or dump
  order.
- Copying or trimming the monolithic `930930-1` shape as proof.
- Expectation rewrites, unsupported-marker downgrades, allowlists, pass/fail
  accounting changes, runtime-comparison changes, or baseline/log churn.
- Touching `review/`, `test_before.log`, `test_after.log`,
  `test_baseline.log`, or `test_baseline.new.log`.

## Working Model

Idea 482 proved the focused destination probe can isolate semantic identity and
frame-slot destination/layout facts while keeping materialization authority and
downstream consumers fail-closed. This plan resumes the original 481 source
intent at the next producer seam.

## Execution Rules

- Step 1 is classification-only unless it identifies an already bounded,
  source-owned producer edit.
- Do not continue from the `%t23` monolith without showing why the focused
  scalar probe is insufficient.
- Any implementation packet must add or consume explicit materialization-point
  authority, not raw placement or naming inference.
- Classification-only proof:

```sh
git diff --check
```

- Code-changing proof, when a producer packet is selected:

```sh
cmake --build build -j2
ctest --test-dir build -j2 --output-on-failure -R '^backend_'
git diff --check
```

## Steps

### Step 1: Audit Focused Materialization Point Inputs

Inspect the focused scalar compare destination probe and current prepared
surfaces. Completion means `todo.md` records which facts are already explicit,
which materialization-point authority is still missing, and whether Step 2 can
define a bounded producer contract from those inputs.

### Step 2: Define Focused Materialization Point Contract

Define the smallest sound contract for accepting a semantic instruction-result
frame-slot materialization point from explicit producer evidence. Completion
means the contract names accepted authority, required identity and destination
fields, and fail-closed rejection statuses for missing authority, storage-only
movement, source/result mismatch, destination mismatch, protected select-result
boundaries, and raw-shape inference.

### Step 3: Implement Or Route Focused Materialization Point Producer

Implement the bounded producer packet if Step 2 identifies one. If current
prepared/prealloc surfaces still cannot provide explicit authority, record the
exact lower-level owner and stop without changing target lowering or downstream
consumers.

### Step 4: Residual Disposition And Close Readiness

Re-probe the focused case and any affected representative evidence. Completion
means lifecycle state says whether 481 is complete, blocked by a precise lower
producer, or ready to resume downstream write-event/source-fact work.
