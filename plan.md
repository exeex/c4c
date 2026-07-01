# Semantic Frame-Slot Materialization Probe Decomposition Plan

Status: Active
Source Idea: ideas/open/482_semantic_frame_slot_materialization_probe_decomposition.md
Deactivates Runbook: ideas/open/481_semantic_result_frame_slot_materialization_point_producer.md

## Purpose

Reset the stuck semantic frame-slot materialization route into focused probes
and explicit backend seam ownership before more implementation work resumes.

## Goal

Replace the monolithic `%t23` route with non-duplicative focused probes that
separate semantic identity, frame-slot destination, materialization authority,
storage-only move rejection, and select-result boundaries.

## Core Rule

Do not continue by auditing the same `%t23` materialization-point gap again.
First establish the blocked family, split it into focused probes, and bind each
probe to one owned backend seam.

## Read First

- ideas/open/482_semantic_frame_slot_materialization_probe_decomposition.md
- ideas/open/481_semantic_result_frame_slot_materialization_point_producer.md
- build/agent_state/475_step4_source_fact_population_residual/disposition.md
- build/agent_state/476_step4_semantic_materialization_interval_residual/disposition.md
- build/agent_state/477_step4_real_semantic_fact_population_residual/disposition.md
- build/agent_state/478_step4_semantic_write_event_carrier_residual/disposition.md
- build/agent_state/479_step4_real_event_authority_residual/disposition.md
- build/agent_state/480_step4_semantic_write_event_producer_residual/disposition.md
- tests/backend/case/

## Current Target

- Blocked representative family:
  - semantic compare result `%t23 = bir.ne i32 %t22, 0`;
  - final frame-slot destination `slot #21`;
  - rejected `%t22 -> %t23` `authority=none` storage movement;
  - `%t22` select-result stack-destination boundary.
- Candidate focused probe families:
  - scalar compare result forced to frame slot;
  - storage-only move rejection;
  - select-result stack-destination boundary;
  - synthetic explicit materialization-point positive probe if representable.

## Non-Goals

- Implementing idea 481 materialization-point production before focused probes
  and seam ownership are established.
- Copying the monolithic `930930-1` shape into `tests/backend/case/`.
- RV64 branch-load emission or target lowering.
- Populating downstream semantic interval, source-fact, or
  `PreparedBranchStackLoadAuthority` rows.
- Pointer-value/provenance repair, select-result stack-destination repair, or
  unsupported-terminator repair except as classified boundaries.
- Inferring materialization authority from raw BIR adjacency, final homes,
  storage, offsets, object ids, value names, function names, testcase names, or
  dump order.
- Expectation rewrites, unsupported-marker downgrades, allowlists, pass/fail
  accounting changes, runtime-comparison changes, or baseline/log churn.
- Touching `review/`, `test_before.log`, `test_after.log`,
  `test_baseline.log`, or `test_baseline.new.log`.

## Working Model

Ideas 475 through 481 kept lowering the missing producer without shrinking the
owned failure family. This plan is a route-quality correction. It should first
produce focused, capability-oriented probes or explicitly record why a probe
cannot yet be represented.

## Execution Rules

- Step 1 is classification-only; do not edit implementation or tests.
- Add focused backend probes under `tests/backend/case/` only after Step 1/2
  identifies non-duplicative cases.
- Each probe must have one primary contract and one backend owner.
- Classification-only proof:

```sh
git diff --check
```

- Probe/test proof, if Step 2/3 selects test additions:

```sh
cmake --build build -j2
ctest --test-dir build -j2 --output-on-failure -R '^backend_'
git diff --check
```

## Steps

### Step 1: Establish The Blocked Failure-Family Baseline

Summarize the 475 -> 481 chain and the current `%t23` family using existing
artifacts. Completion means `todo.md` records the concrete repeated symptoms,
the monolithic owner family, and the seams that must not be conflated.

### Step 2: Split The Monolithic Probe Into Focused Cases

Identify focused backend probe candidates under `tests/backend/case/` without
copying the monolith. Completion means `todo.md` lists accepted/rejected probe
candidates and why each accepted probe is non-duplicative.

### Step 3: Bind Each Focused Case To One Owned Backend Seam

Map each focused case to exactly one seam and backend owner: semantic identity,
destination facts, materialization authority, storage-only move rejection,
select-result boundary, or downstream non-goal. Completion means each probe has
one expected first owner and one fail-closed boundary.

### Step 4: Resume Implementation On The Narrowest Generic Seam

Choose the narrowest generic seam that is ready for implementation, or record
why no seam is currently executable. Completion means lifecycle state either
hands a focused implementation packet to the executor or routes a precise
follow-up idea without returning to monolithic `%t23` chasing.
