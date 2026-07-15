# LIR Cast Result Authority Baseline Residual Runbook

Status: Active
Source Idea: ideas/open/796_lir_instruction_terminator_residual_authority_handoff.md
Activated from: 810 Step 3 comparable-baseline residual ownership switch

## Purpose

Trace the selected native `LirCastOp.result` raw-text authority failure family
that prevents the 810 comparable full baseline from clearing. This is a
bounded 796 instruction-authority route, not a GEP, structured-call, or PHI
repair.

## Core Rule

Use only native current-function value/type facts. Do not parse rendered text,
weaken verification, group cases by testcase identity, or convert unrelated
instruction families.

## Read First

- `ideas/open/796_lir_instruction_terminator_residual_authority_handoff.md`
- `ideas/open/810_lir_gep_producer_result_authority_baseline_blocker.md`
- the full-gate diagnostics for representative `LirCastOp.result` failures

## Non-Goals

- 810's accepted GEP repair, 795's parameter handoff, or 810 baseline
  clearance.
- 801's `frontend_lir_call_type_ref` structured-call/argument-mirror route.
- 806's `20060910-1.c` PHI route, a combined residual sweep, Raw-BIR, or
  inline-assembly text parsing.

## Ordered Steps

### Step 1 - Trace and select the native cast-result authority family

Goal: establish whether the 22 raw-text diagnostics share one native cast
result publication/handoff and identify the smallest selected producer seam.

Actions:

- reproduce representative positive, c-testsuite, and GCC-torture cases;
- trace `LirCastOp.result` from native production through verification;
- record explicit ownership for every nonmatching residual before expanding.

Completion check: a single evidenced native family is selected, or each
nonmatching case is routed to an existing/new separately scoped successor.

### Step 2 - Repair the selected cast-result handoff

Goal: publish checked native authority to the existing cast contract.

Actions:

- implement only the selected producer/immediate handoff;
- add nearby same-family positive and malformed-authority coverage;
- retain rejection of missing, foreign, stale, and unknown authority.

Completion check: fresh build and focused proof pass without verifier
weakening, text recovery, or testcase-shaped behavior.

### Step 3 - Prove the bounded route and return to 810

Goal: provide 796 acceptance evidence sufficient to retry 810's gate.

Actions:

- obtain supervisor-selected focused proof for the selected family;
- have the supervisor classify/accept the result;
- reactivate 810 unchanged at its preserved Step 3 for
  `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure`.

Completion check: 796's bounded route is accepted or explicitly split; no
claim of 810 or 801 full-baseline clearance is made here.
