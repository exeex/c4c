# Semantic Frame-Slot Materialization Probe Decomposition

Status: Open
Type: Decomposition and focused-probe planning idea
Supersedes Active Runbook: `ideas/open/481_semantic_result_frame_slot_materialization_point_producer.md`
Source Evidence:
- `build/agent_state/475_step4_source_fact_population_residual/`
- `build/agent_state/476_step4_semantic_materialization_interval_residual/`
- `build/agent_state/477_step4_real_semantic_fact_population_residual/`
- `build/agent_state/478_step4_semantic_write_event_carrier_residual/`
- `build/agent_state/479_step4_real_event_authority_residual/`
- `build/agent_state/480_step4_semantic_write_event_producer_residual/`
Owning Layer: Backend semantic frame-slot materialization route decomposition

## Goal

Break the blocked semantic frame-slot materialization route into focused probes
and owned backend seams before any more implementation work resumes.

## Why This Exists

The recent 475 -> 481 lifecycle chain kept moving the first missing producer
lower without reducing the representative `%t23` failure family:

- source facts;
- semantic interval;
- write-event carrier;
- real event authority;
- write event producer;
- materialization point.

The active route is therefore stuck by the c4c divide-and-conquer rule. A new
audit of the same monolithic `930930-1` shape would likely keep chasing the
same moving first bad fact. This idea resets the route around smaller probes
and explicit ownership seams.

## Seams To Split

- Semantic instruction-result identity versus frame-slot destination facts.
- Actual materialization point and producer authority.
- Storage-only move rejection for `authority=none`, including `%t22 -> %t23`.
- Select-result stack-destination boundary for `%t22`.
- Downstream interval, source-fact, branch-stack-load authority, and RV64
  consumers as separate non-goals until a producer seam is proven.

## Expected Probe Direction

Focused backend probes should live under `tests/backend/case/` only after Step
1/2 identify non-duplicative cases. Do not copy the full `930930-1` shape.
Candidate probe families:

- Scalar compare result forced to a frame slot.
- Storage-only move rejection with `authority=none`.
- Select-result stack-destination boundary.
- Synthetic explicit materialization-point positive probe, if representable
  without raw-shape inference.

## In Scope

- Establish a compact baseline of the blocked failure family from existing
  artifacts.
- Decide which focused probes are non-duplicative and capability-oriented.
- Bind each probe to exactly one backend seam.
- Decide the narrowest generic implementation seam after focused probes or
  recorded non-representability.
- Keep idea 481 parked until decomposition identifies the next executable seam.

## Out Of Scope

- Implementing semantic materialization-point production before focused probes
  and seam ownership are established.
- Copying the monolithic `930930-1` testcase into a smaller file without
  isolating one primary contract.
- RV64 branch-load emission or target lowering.
- Populating downstream semantic interval, source-fact, or
  `PreparedBranchStackLoadAuthority` rows.
- Pointer-value/provenance repair, select-result stack-destination repair, or
  unsupported-terminator branch-site relationship repair except as classified
  boundaries.
- Inferring materialization authority from raw BIR adjacency, final stack
  homes, storage, offsets, object ids, value names, function names, testcase
  names, or dump order.
- Expectation rewrites, unsupported-marker downgrades, allowlists, pass/fail
  accounting changes, runtime-comparison changes, or baseline/log churn.

## Acceptance Criteria

- Step 1 records the blocked failure-family baseline without selecting another
  same-shape implementation packet.
- Step 2 identifies focused probe candidates and rejects duplicate monolith
  copies.
- Step 3 maps each accepted focused probe to one backend seam and one owner.
- Step 4 selects the narrowest generic seam for implementation or records why
  no probe is currently representable.
- Idea 481 remains parked or is resumed only with a focused, non-duplicative
  seam.

## Reviewer Reject Signals

- Reject patches that continue the `%t23` route by adding another lower-level
  producer idea without focused probes or seam ownership.
- Reject focused probes that are only renamed or slightly trimmed copies of the
  monolithic `930930-1` shape.
- Reject using raw BIR, final homes, storage, object ids, value names, function
  names, testcase names, or dump order as materialization authority.
- Reject RV64 lowering, downstream authority consumption, expectation rewrites,
  unsupported downgrades, allowlists, baseline churn, or classification-only
  changes claimed as backend capability progress.
