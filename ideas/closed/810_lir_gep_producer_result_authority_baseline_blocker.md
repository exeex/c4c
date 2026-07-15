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

## Resumption Record: function-body parameter index blocker

- Last accepted progress: Step 1, *Trace and classify failing GEP producer
  families*, was accepted in `f1cb9c510`; Step 2, *Repair the selected GEP
  result-authority handoff*, was accepted in `1f1a1fb38`. The accepted repair
  is limited to native-immediate RHS pointer compound add/sub together with
  the selected postfix producer family.
- Interrupted runbook step: Step 3, *Prove the blocker and return control to
  801*.
- Baseline rejection and scope boundary: the hook-managed full baseline after
  `1f1a1fb38` passed 3035/3037 rather than 3037/3037. The `20060910-1.c` PHI
  failure remains exclusively with the existing 806 PHI residual-producer
  route. `pr21173.c` reaches the selected direct GEP with a variable RHS whose
  index is a non-native function-body parameter authority; that is not the
  native-immediate GEP producer repair accepted in Step 2 and must not be
  solved by text recovery or verifier weakening.
- Blocking owner: existing
  `ideas/open/795_lir_body_parameter_authority_handoff.md` owns the bounded
  native body-parameter/index authority and ABI-form classification route.
- Exact return point: after 795 provides supervisor-accepted focused proof for
  the selected parameter-index handoff, reactivate 810 unchanged at Step 3
  only. Do not repeat or reopen accepted Steps 1--2; then obtain a fresh,
  comparable 3037/3037 full baseline before returning 801 unchanged at its
  Step 2 gate.
- Accepted proof and implementation references: Step 2's focused
  `frontend_lir_call_type_ref` guard changed from 0/1 before to 1/1 after the
  fresh build; the matching non-decreasing guard was accepted. This is bounded
  producer proof, not a full-baseline clearance.

## Resumption Record: comparable baseline residual ownership switch

- Last accepted progress: Step 1 remains accepted in `f1cb9c510` and Step 2
  remains accepted in `1f1a1fb38`; `f3c16c57f` preserved the resumed 810
  baseline gate, and the bounded native function-body parameter index handoff
  completed separately in `281737387`. No accepted 810 work is reopened by
  this record.
- Interrupted runbook step: Step 3, *Prove the blocker and return control to
  801*.
- Exact failed proof: after temporarily stashing and then restoring the
  preserved dirty aggregate/direct-complex working-tree changes, the isolated
  accepted 810/795 tree ran `cmake --build --preset default && ctest --test-dir
  build -j --output-on-failure`. The build succeeded; CTest passed 3013/3037
  and failed 24 tests. This is a fresh comparable failed gate, not a
  regression-guard roll-forward and not 3037/3037 clearance.
- Residual ownership and scope boundary: `frontend_lir_call_type_ref` remains
  within 801's direct-complex structured-call/argument-mirror Step 2 route;
  `llvm_gcc_c_torture_src_20060910_1_c` remains exclusively with 806's PHI
  residual-producer route. The remaining 22 failures all report
  `LirCastOp.result: expected operand kind mismatch ... got raw-text`, across
  positive/C++/c-testsuite/GCC-torture coverage. That is the existing 796
  bounded residual instruction authority family, not GEP producer work. The
  isolated-tree result proves the preserved 801 dirty hunks did not cause this
  failed gate.
- Blocking owner and exact return point: switch to
  `ideas/open/796_lir_instruction_terminator_residual_authority_handoff.md`
  to select and trace only the native cast-result authority family. After 796
  completes its bounded route and its result is classified or accepted, resume
  810 unchanged at Step 3; rerun the exact full command above. Return control
  to 801 unchanged at Step 2 only after the supervisor accepts 3037/3037.

## Accepted Completion and Parent Return

Disposition: capability complete. Step 1's bounded GEP producer trace was
accepted in `f1cb9c510`; Step 2's selected producer-result authority repair
was accepted in `1f1a1fb38`. The intervening bounded parameter and cast
handoffs completed in `281737387` and `387af7745` without reopening either
accepted 810 step.

Supervisor acceptance evidence: on 2026-07-15 at 15:51 UTC, the exact
comparable gate `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure` completed successfully with 3037/3037 tests passed.
This is the accepted Step 3 proof and clears only 810's bounded baseline
blocker.

Parent return: resume
`ideas/open/801_lir_anonymous_aggregate_layout_type_facts.md` unchanged at
Step 2, `Repair anonymous layout / structured-call compatibility`. Preserve
the unaccepted working-tree repair in `call/args.cpp`, `call/target.cpp`,
`verify.cpp`, and `frontend_hir_tests.cpp`; evaluate it under 801's existing
scope before any Step 2 acceptance. Do not repeat 801 Step 1 or credit those
dirty hunks as accepted progress through this closure.
