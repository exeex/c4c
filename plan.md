# RV64 Loop-Carried Pointer Post-Increment Publication Plan

Status: Active
Source Idea: ideas/open/323_rv64_loop_carried_pointer_postincrement_publication.md

## Purpose

Repair RV64 prepared lowering/publication for loop-carried pointer
post-increment values across Duff's-device-style fallthrough blocks.

## Goal

Make repeated iterations advance from current `from`/`to` pointer values
instead of rematerializing fixed array-base offsets, without matching
`src/00143.c`, Duff's-device spelling, block names, stack offsets, or SSA
names.

## Core Rule

This route owns loop-carried pointer post-increment publication only. Empty
loop-exit successor emission, i16 halfword local-array publication,
stack-homed fused compare control flow, and nested store-source publication
are already separate closed routes unless fresh evidence proves a regression.

## Read First

- `ideas/open/323_rv64_loop_carried_pointer_postincrement_publication.md`
- Idea 322 Step 4 artifacts under
  `build/rv64_c_testsuite_probe_latest/triage_322_step4/`
- Focused coverage from ideas 319, 321, and 322 so this route does not reopen
  already advanced control-flow or halfword-local-array behavior.

## Scope

- RV64 prepared lowering/publication for loop-carried pointer post-increment
  values.
- Pointer local updates across Duff's-device-style fallthrough blocks and
  repeated loop iterations.
- Focused backend coverage proving pointer post-increment locals advance from
  current values rather than fixed array-base offsets.
- Candidate reprobe for `src/00143.c` after empty-exit successor emission and
  i16 local-array select/store publication have already advanced.

## Non-Goals

- Empty loop-exit successor emission from idea 322.
- I16 local-array select/store publication from idea 321.
- Stack-homed fused compare control flow from idea 319.
- Nested select-chain/store-source publication for `src/00144.c` from idea
  320.
- Aggregate/byval repair, function-pointer repair, external-call policy, or
  broad switch/control-flow rewrites beyond loop-carried pointer
  post-increment publication.

## Working Model

After idea 322, `src/00143.c` emits and links without the old fallthrough
`SIGILL`, but qemu exits 1. The emitted Duff's-device copy blocks repeatedly
store fixed stack addresses back into `%lv.from` and `%lv.to` homes, such as
`addi t1, sp, 2; sd t1, 160(sp)` and `addi t1, sp, 80; sd t1, 168(sp)`.
Prepared-BIR shows the same issue: values such as
`%t29 = bir.add ptr %lv.a.0, 2` are stored to `%lv.from`, so repeated
iterations restart from array-base-derived addresses instead of current
loop-carried pointers.

## Execution Rules

- Use prepared pointer/local publication facts as the source of authority.
- Do not match `src/00143.c`, Duff's-device source spelling, `%lv.from`,
  `%lv.to`, `%t29`, fixed stack offsets, block names, or fixed array sizes.
- Do not avoid repeated fallthrough loop iterations in focused fixtures just
  to make proof pass.
- Keep runtime proof on the supported path; do not mark the candidate
  unsupported or weaken qemu coverage to claim progress.
- Validation ladder for implementation steps: build, focused backend proof,
  candidate reprobe, then backend regression guard when closure is proposed.

## Step 1: Normalize Loop-Carried Pointer Evidence

Goal: reproduce the current `src/00143.c` wrong-result and identify the first
loop-carried pointer post-increment publication failure.

Actions:

- Reprobe `src/00143.c` through BIR, prepared-BIR, RV64 emit, clang link, and
  qemu.
- Confirm already-closed boundaries still advance: empty-exit successor
  emission, i16 halfword local-array publication, and stack-homed fused compare
  control flow.
- Inspect prepared pointer/local updates around `%lv.from` and `%lv.to` and
  emitted stores to their stack homes.
- Identify whether post-increment values are derived from current loop-carried
  pointer locals or rematerialized from fixed array bases.

Completion checks:

- Fresh artifacts identify the first pointer post-increment publication or
  lowering failure.
- The evidence distinguishes this route from ideas 319, 321, and 322.
- `todo.md` records candidate artifacts, proof command, and classification.

## Step 2: Add Focused Loop-Carried Pointer Coverage

Goal: create focused backend coverage for repeated pointer post-increment
locals across fallthrough/repeated loop iterations.

Actions:

- Add a focused fixture with repeated iterations where pointer locals must
  advance from current values across fallthrough blocks.
- Assert prepared facts strongly enough to catch rematerialization from fixed
  array-base offsets.
- Assert generated RV64 strongly enough to catch stores of fixed addresses back
  into pointer local homes.
- Include runtime proof that fails when repeated iterations restart from fixed
  base-derived pointers.

Completion checks:

- Focused dump/codegen/runtime coverage captures the current pointer
  post-increment gap or uses the repo's expected-repair convention.
- The fixture does not depend on `src/00143.c`, Duff's-device spelling, fixed
  stack offsets, block names, SSA names, or array sizes.
- Closed-route neighbors remain green.

## Step 3: Repair Prepared Pointer Post-Increment Publication

Goal: preserve loop-carried pointer post-increment values through prepared
pointer/local publication.

Actions:

- Locate where pointer local update facts are built or propagated for
  post-increment values across fallthrough blocks.
- Ensure updates consume the current pointer local value when required rather
  than rematerializing from a fixed array base.
- Preserve source/ordering facts so repeated iterations see the advanced
  pointer state.
- Avoid target-local source scans or testcase-shaped fallback matching.

Completion checks:

- Focused prepared dump coverage shows pointer locals advance from current
  loop-carried values.
- The repair is not tied to candidate filenames, Duff's-device spelling, SSA
  names, fixed offsets, or fixed array sizes.
- Neighbor coverage for already closed routes remains green.

## Step 4: Repair RV64 Consumption and Runtime Behavior

Goal: ensure RV64 lowering consumes the repaired pointer publication facts and
emits correct post-increment behavior across repeated iterations.

Actions:

- Update RV64 lowering only where it consumes prepared pointer/local
  publication facts.
- Emit pointer local updates based on current loop-carried pointer values.
- Run focused dump/codegen/runtime proof and reprobe `src/00143.c`.
- If `src/00143.c` advances to a distinct residual, classify it rather than
  expanding this plan silently.

Completion checks:

- Focused runtime coverage passes because repeated iterations advance pointer
  locals correctly.
- `src/00143.c` either emits, links, and runs under qemu, or has a concrete
  out-of-scope residual classification.
- Generated assembly no longer stores fixed array-base rematerializations back
  into pointer local homes in the focused proof cases.

## Step 5: Reprobe and Close Classification

Goal: decide whether idea 323 is complete under its source acceptance criteria.

Actions:

- Re-run focused backend proof and the `src/00143.c` candidate probe.
- Compare against existing backend regression logs or generate a matching close
  guard if closure is proposed.
- Create a separate follow-up idea only for a newly exposed residual outside
  this source idea.

Completion checks:

- Focused loop-carried pointer post-increment coverage is green.
- `src/00143.c` is either `supported-linked-pass` or has a concrete
  out-of-scope residual classification.
- Close-time regression guard evidence is ready for plan-owner review.
