# RV64 Stack Homed Fused Compare Control Flow Plan

Status: Active
Source Idea: ideas/open/319_rv64_stack_homed_fused_compare_control_flow.md

## Purpose

Repair RV64 prepared control-flow emission when branch or loop conditions use
fused compare values that have been stack-homed instead of already residing in
registers.

## Source Intent

Idea 319 exists because idea 316 Step 5 separated `src/00077.c` and
`src/00143.c` from pointer-array/select work. Both candidates reach prepared
control-flow paths where the compare feeding a branch is stack-homed. The
repair must make those prepared compare/control facts usable by RV64 emission
without falling through into the next function or truncating assembly.

## Scope

- RV64 prepared branch emission for stack-homed fused compare conditions.
- RV64 prepared loop-condition emission for stack-homed fused compare
  conditions.
- Focused backend coverage for the branch and loop shapes exposed by
  `src/00077.c` and `src/00143.c`.
- Emitted-code, link, and qemu proof that separates compare/control-flow
  repair from local-array, pointer publication, aggregate, byval, function
  pointer, and external-call work.

## Non-Goals

- Reopening pointer-to-pointer local frame-address materialization from idea
  316.
- Repairing nested select-chain/store-source publication from `src/00144.c`.
- Broad switch lowering beyond what is necessary for stack-homed fused compare
  branch or loop conditions.
- Aggregate/byval, function-pointer, or external-call policy repair.

## Execution Rules

- Use prepared compare/control-flow facts and stack-home information. Do not
  match filenames, SSA names such as `%t5` or `%t1`, observed stack offsets, or
  source shapes.
- Prove the first stack-homed compare branch or loop condition before pursuing
  later local-array/select behavior exposed by the same testcase.
- Do not claim progress by weakening runtime contracts, skipping qemu, or
  marking the candidates unsupported.
- Keep proof focused but include a nearby same-feature shape so the route does
  not overfit one named candidate.

## Step 1: Normalize Stack-Homed Fused Compare Evidence

Reproduce the current failure family for `src/00077.c` and `src/00143.c`.
Capture BIR, prepared-BIR, emitted RV64, clang link, and qemu status for each
candidate. Identify the first bad emitted-code fact for each case, including
whether emission falls through into the next function, truncates before a
branch, or reaches a distinct later mechanism.

Completion checks:

- `src/00077.c` and `src/00143.c` have fresh, comparable evidence.
- The stack-homed fused compare condition and its control-flow consumer are
  identified from prepared facts.
- Any later local-array/select behavior is explicitly deferred unless it is the
  first remaining failure after compare/control-flow emission is repaired.

## Step 2: Add Focused Stack-Homed Compare Control Coverage

Add or update focused backend tests that exercise RV64 branch and loop
conditions fed by stack-homed fused compare values. The tests should fail on
fall-through or truncation and should cover the source shapes represented by
`src/00077.c` and `src/00143.c` without hard-coding those files.

Completion checks:

- Focused coverage distinguishes branch emission from loop-condition emission.
- Tests assert generated control flow strongly enough to catch missing branch
  or return paths.
- Runtime proof remains part of the contract for supported-linked candidates.

## Step 3: Repair Stack-Homed Fused Compare Branch Emission

Update the RV64 prepared branch emission path so a stack-homed fused compare
condition can be loaded or materialized into a usable control-flow value.
Preserve existing compare publication and prepared edge behavior while removing
the fall-through behavior observed in the entry-branch candidate.

Completion checks:

- The focused branch test passes.
- `src/00077.c` emits complete RV64 assembly, links, and either runs under qemu
  or is reclassified with concrete emitted-code evidence as a different
  mechanism.
- The repair is data-flow driven and contains no filename, SSA-name, or offset
  special case.

## Step 4: Repair Stack-Homed Fused Compare Loop-Condition Emission

Extend the same capability to loop-condition control flow. Ensure the prepared
loop branch can consume stack-homed compare operands or results without
truncating assembly before the loop body or continuation path.

Completion checks:

- The focused loop-condition test passes.
- `src/00143.c` emits complete RV64 assembly through the first loop condition,
  links, and either runs under qemu or exposes a separately classified later
  mechanism.
- Any newly exposed local-array/select behavior is recorded as a residual only
  after compare/control-flow emission is proven.

## Step 5: Reprobe and Close Classification

Re-run the focused backend proof and the candidate probe for `src/00077.c` and
`src/00143.c`. Decide whether idea 319 is complete, whether a residual belongs
to another existing/open idea, or whether a new follow-up idea is required.

Completion checks:

- Focused stack-homed fused compare branch and loop coverage is green.
- Each candidate is either `supported-linked-pass` or has a concrete residual
  classification that is outside idea 319 scope.
- Regression guard evidence is ready for lifecycle close review.
