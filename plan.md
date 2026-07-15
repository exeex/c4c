# LIR Cast Result Authority Recurrence Runbook

Status: Active
Source Idea: ideas/open/796_lir_instruction_terminator_residual_authority_handoff.md
Resumed from: 801 Step 2 comparable-full-gate blocker switch.

## Purpose

Determine whether the newly re-exposed `LirCastOp.result` raw-text failures
belong to 796's previously selected cast producer family, then make only the
bounded native-authority repair that evidence supports.

## Core Rule

Use native value and type authority only. Do not parse rendered text, weaken
the verifier, classify behavior by testcase name, or absorb 801's anonymous
layout/direct-complex structured-call work.

## Accepted History

Steps 1--3 of the original cast route are accepted: trace in `e92437aca`,
scalar `StmtEmitter::coerce` handoff repair in `387af7745`, and its focused
proof/regression guard recorded in the source idea. This recurrence does not
reopen or re-credit that accepted scalar route.

## Non-Goals

- 801 structured-call/anonymous-layout repair or acceptance.
- 810/795 GEP/parameter work, 806 PHI work, Raw-BIR, combined residual
  sweeps, or inline-assembly template/constraint parsing.
- Treating the capped `test_after.log`, non-clean 24-failure before attempt,
  or stale `test_baseline.new.log` as a baseline.

## Ordered Steps

### Step 4 - Re-trace the recurrent cast-result raw-text family (accepted)

Goal: establish whether representative current `LirCastOp.result` raw-text
failures are an unhandled native cast producer/handoff within 796, a regression
of the accepted scalar seam, or a separately owned family.

Actions:

- reproduce representative positive, LLVM, and c-testsuite failures without
  modifying 801's preserved hunks;
- trace each result from producer through `verify_cast_op_authority` using
  native current-function facts;
- compare the producer shape with accepted scalar `StmtEmitter::coerce` work
  and explicitly route every nonmatching family.

Completion check: accepted as a trace/selection checkpoint. A fresh build and
the positive, LLVM, and c-testsuite representatives all selected only the
non-parameter pointer-subtraction producer/immediate-handoff in
`expr/binary.cpp`: raw `fresh_tmp` `PtrToInt` results and the dependent
subtraction/scaling results reach `verify_result_operand` before cast-specific
authority checks. The scalar `StmtEmitter::coerce` route is distinct and stays
accepted. No repair is accepted and no full baseline is used at this step.

### Step 5 - Repair the selected recurrent cast-result handoff (accepted)

Goal: publish native value authority for only the selected non-parameter
pointer-subtraction producer/immediate-handoff seam.

Actions:

- give the selected `PtrToInt` results and their dependent
  subtraction/scaling result values native `fresh_value` authority;
- add nearby same-family positive and malformed-authority coverage;
- retain the verifier and accepted scalar `StmtEmitter::coerce` route
  unchanged.

Completion check: accepted by the supervisor. Both `PtrToInt` results, the
immediate byte subtraction, and optional element-size scaling now use
`fresh_value` authority in `expr/binary.cpp`. The fresh focused proof passed
4/4 without text recovery, verifier weakening, or testcase-shaped behavior;
no canonical regression guard was made.

### Step 6 - Accept the bounded 796 repair and return to 801

Goal: supply supervisor-accepted 796 evidence and make a fresh comparable
baseline route available to 801 Step 2.

Actions:

- record the supervisor-accepted Step 5 proof against its bounded contract;
- obtain or explicitly route the fresh comparable clean baseline needed for
  the interrupted 801 gate; do not use the rejected/capped historical logs;
- reactivate 801 unchanged at Step 2 only after the bounded 796 repair is
  accepted and a comparable baseline permits the retry.

Completion check: accepted 796 proof and an explicit return record exist; no
claim that 801 Step 2 or its full gate has passed.
