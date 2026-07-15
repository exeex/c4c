# LIR GEP Producer Result Authority Baseline Blocker

Status: Open
Type: bounded LIR GEP producer-authority baseline blocker
Blocked Parent: `ideas/open/801_lir_anonymous_aggregate_layout_type_facts.md`

## Goal

Trace the LIR producer handoff behind the full-baseline `LirGepOp.result`
authority failures and repair only the evidenced GEP producer family so the
existing authoritative GEP verifier contract receives a valid current-function
`LirValueId`.

## Why This Exists

During 801 Step 2 acceptance, a fresh build and focused
frontend/call/backend ladder passed (2/2), but the required full-baseline
attempt exposed 115 frontend failures at `LirGepOp.result: authoritative GEP
requires LirValueId result authority`. This is a producer-side authority
family, not anonymous aggregate layout or structured-call compatibility. The
partial root `test_after.log` is not a comparable regression guard: execution
timing stopped it, and its clean-worktree baseline lacked external assets and
a summary. A residual PHI failure remains separately owned by 804/806.

## In Scope

- Reproduce and trace representative GEP failures from each observed producer
  family to the immediate LIR result-publication or lowering handoff.
- Group cases only when native producer/handoff evidence establishes the same
  failure mechanism.
- Make the smallest producer-side or immediate handoff repair that supplies a
  checked current-function `LirValueId` to the existing authoritative GEP
  verifier.
- Add nearby same-family positive and malformed-authority coverage, run
  focused proof, and provide the evidence needed to retry 801's full-baseline
  gate.

## Out Of Scope

- Anonymous aggregate layout, direct-complex call signature/argument mirrors,
  or any acceptance of 801's preserved Step 2 working-tree repair.
- Reopening or weakening `verify_authoritative_gep`, using rendered text or
  instruction order as identity, generic provenance rewrites, Raw-BIR,
  pointer/object/memory redesign, MIR/emission, or non-GEP producer families.
- PHI residual authority failures, which remain with the 804/806 chain.
- Treating a partial baseline or root regression log as parent acceptance.

## Acceptance Criteria

- Every selected failure family is tied to an evidenced GEP producer/result
  handoff, or is explicitly split into a separately scoped successor before
  repair broadens beyond that family.
- The selected producer publishes a valid current-function `LirValueId` while
  missing, foreign, stale, and unknown result authority remain rejected by the
  existing GEP contract.
- Nearby same-feature positive and malformed coverage plus fresh focused proof
  pass, and the supervisor has sufficient accepted evidence to reactivate 801
  unchanged at Step 2 for a comparable full-baseline reattempt.

## Accepted Trace Checkpoint

Step 1 trace-only evidence was accepted in `f1cb9c510`
(`review/810_step1_gep_producer_trace.md`). The exact focused command
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^c_testsuite_src_00173_c$'`
reproduced the missing `LirGepOp.result` authority in
`c_testsuite_src_00173_c`. The selected repair family is native pointer postfix
increment/decrement and adjacent pointer compound add/sub only: postfix
increment loads its base through `fresh_value(ctx)` and forms the GEP result
through `fresh_tmp(ctx)` at `lvalue.cpp:645-649`, with the adjacent `+=`/`-=`
route using the same direct construction at `lvalue.cpp:737-741`. Other
partial-log GEP failures remain ungrouped pending independent producer traces.

## Reviewer Reject Signals

- Reject weakening `LirGepOp.result` or `verify_authoritative_gep`, accepting
  text-derived identity, or using testcase-name/instruction-order shortcuts to
  make the named failures pass.
- Reject assuming all 115 diagnostics share one producer route without trace
  evidence, or absorbing a separate producer family without a successor.
- Reject broad provenance, pointer, Raw-BIR, aggregate-layout, call, or PHI
  rewrites claimed as a bounded GEP result-authority repair.
- Reject expectation downgrades, classification-only changes, named-case-only
  coverage, or any claim that the partial baseline establishes regression
  status or parent clearance.

## Parent Return Contract

801 Step 1 is accepted in `827dae5bd3`; its Step 2 repair is preserved,
reviewed scope-consistent, unaccepted, and uncommitted. Once this blocker has
accepted its bounded GEP authority repair and proof sufficient for a full
baseline reattempt, reactivate 801 at unchanged Step 2. Then the supervisor
must rerun a fresh build, the focused
`^(frontend_hir_tests|frontend_lir_call_type_ref|backend_)$` ladder, and a
comparable supervisor-accepted full baseline before Step 3. Do not repeat 801
Step 1 or credit its preserved repair through this blocker.
