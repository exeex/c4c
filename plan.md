# LIR GEP Producer Result Authority Baseline Blocker Runbook

Status: Active
Source Idea: ideas/open/810_lir_gep_producer_result_authority_baseline_blocker.md
Activated from: 801 Step 2 full-baseline authority blocker; return to 801 only after accepted bounded proof.

## Purpose

Restore the native result-authority handoff for the evidenced LIR GEP producer
family without weakening the authoritative GEP verifier or absorbing 801's
anonymous-layout/structured-call work.

## Core Rule

`LirGepOp.result` must carry native current-function `LirValueId` authority.
Do not derive authority from rendered text, testcase identity, or instruction
order, and do not weaken the existing verifier contract.

## Read First

- `ideas/open/810_lir_gep_producer_result_authority_baseline_blocker.md`
- `ideas/open/801_lir_anonymous_aggregate_layout_type_facts.md` resumption record
- nearby authoritative GEP construction and `verify_authoritative_gep`
- current full-baseline failure evidence, treating partial logs as diagnostic only

## Non-Goals

- 801 anonymous layout/structured-call repair and its acceptance.
- Generic provenance, pointer/object/memory, Raw-BIR, MIR/emission, or PHI work.
- Any weakening of GEP verifier result authority or text-derived recovery.

## Ordered Steps

### Step 1 - Trace and classify failing GEP producer families

Goal: reproduce representative failures and identify the native producer or
immediate handoff that leaves an authoritative GEP result without a valid
current-function `LirValueId`.

Actions:

- trace from failing GEP verification to result construction/publication;
- group failures only when their producer route is evidenced identical;
- split an unshared family into a separate open successor rather than growing
  this runbook.

Completion check: one bounded producer family is selected with evidence, or
each unshared family has an explicit successor; no verifier weakening or
801/804/806 work is selected.

Accepted: `f1cb9c510` records the trace-only packet in
`review/810_step1_gep_producer_trace.md`. The exact focused command
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^c_testsuite_src_00173_c$'`
reproduced the missing `LirGepOp.result` authority. The selected shared family
is pointer postfix increment/decrement and adjacent pointer compound add/sub:
the postfix route loads its base through `fresh_value(ctx)` and constructs the
GEP result through `fresh_tmp(ctx)` at `lvalue.cpp:645-649`; the adjacent
`+=`/`-=` route has the same direct construction at `lvalue.cpp:737-741`.

### Step 2 - Repair the selected GEP result-authority handoff

Goal: publish the smallest checked native `LirValueId` result authority needed
by the existing authoritative GEP contract for pointer postfix
increment/decrement and adjacent pointer compound add/sub only.

Actions:

- repair only the evidenced producer-side/immediate handoff in the selected
  pointer postfix increment/decrement and adjacent pointer compound add/sub
  family;
- retain rejection of missing, foreign, stale, and unknown result authority;
- add nearby same-family positive and malformed-authority coverage.

Completion check: focused coverage demonstrates the selected pointer
increment/decrement and compound add/sub producers supply valid
current-function authority and malformed forms still reject.

### Step 3 - Prove the blocker and return control to 801

Goal: provide accepted focused proof sufficient for 801 to retry its required
comparable full baseline.

Actions:

- run a fresh build and selected focused same-feature proof;
- have the supervisor assess the evidence and reactivate 801 unchanged at
  Step 2; do not call a partial baseline a regression guard or parent clearance.

Completion check: the supervisor accepts the bounded proof and 801 can resume
its exact Step 2 full-baseline gate without rerunning Step 1.
