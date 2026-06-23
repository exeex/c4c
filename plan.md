# RV64 Empty Loop-Exit Successor Emission Plan

Status: Active
Source Idea: ideas/open/322_rv64_empty_loop_exit_successor_emission.md

## Purpose

Repair RV64 function/control-flow emission when a reachable empty loop-exit
successor sits at the end of emitted text and must still own a valid
return/epilogue path instead of falling through into the next linked section.

## Goal

Make RV64 emission terminate reachable empty end-of-function successor blocks
correctly, using prepared/function control-flow facts and reachability rather
than matching `src/00143.c`, `.Lmain_block_2`, section names, or observed
instruction addresses.

## Core Rule

The i16 halfword local-array body is already out of scope and was repaired by
idea 321. This route owns the later control-flow fact: a reachable loop false
successor with no body instructions must still emit or share a valid
return/epilogue path.

## Read First

- `ideas/open/322_rv64_empty_loop_exit_successor_emission.md`
- Idea 321 Step 3/4 artifacts under
  `build/rv64_c_testsuite_probe_latest/triage_321_step3/`
- Existing focused tests for stack-homed fused compare control flow and i16
  local-array select/store publication, so this route does not reopen those
  closed boundaries.

## Scope

- RV64 emission for empty loop-exit successor blocks.
- Return/epilogue ownership when a reachable successor block has no body
  instructions of its own.
- Focused backend coverage for branches to empty end-of-function successor
  blocks that must not fall through into unrelated sections.
- Candidate reprobe for `src/00143.c` after the i16 local-array body has
  already emitted.

## Non-Goals

- I16 local-array select/store publication from idea 321.
- Stack-homed fused compare branch or loop-condition repair from idea 319.
- Nested select-chain/store-source publication for `src/00144.c` from idea
  320.
- Aggregate/byval repair, function-pointer repair, external-call policy, or
  broad switch/control-flow rewrites beyond empty exit successor emission.

## Working Model

After idea 321, `src/00143.c` emits and links with semantic `lh`/`sh` halfword
select/store bodies. qemu exits 132 because the reachable loop false successor
`.Lmain_block_2` is defined at the end of emitted text with no return/epilogue
body. The branch can jump to that label, and execution falls through into the
next linked section. The repair should ensure empty reachable exit successors
own a valid function exit path.

## Execution Rules

- Use prepared/function control-flow facts, block reachability, and function
  exit/return information as the source of authority.
- Do not match `src/00143.c`, `.Lmain_block_2`, `_IO_stdin_used`, fixed block
  order, linked section layout, or observed instruction addresses.
- Do not weaken the idea 321 i16 focused tests or claim progress through
  unsupported/runtime-skipped expectations.
- Keep proof focused on empty loop-exit successor emission; classify any later
  residual separately instead of silently expanding this route.
- Validation ladder for implementation steps: build, focused backend proof,
  candidate reprobe, then backend regression guard when closure is proposed.

## Step 1: Normalize Empty Exit Successor Evidence

Goal: reproduce the current `src/00143.c` qemu 132 failure and identify the
first empty-exit control-flow emission fact.

Actions:

- Reprobe `src/00143.c` through BIR, prepared-BIR, RV64 emit, clang link, and
  qemu.
- Confirm the i16 local-array body emits semantic `lh`/`sh` before the runtime
  failure.
- Inspect emitted labels, branch targets, and function epilogue/return paths
  around the empty loop false successor.
- Record whether the successor is reachable, has no body instructions, and
  falls through into the next linked section.

Completion checks:

- Fresh artifacts identify the first empty loop-exit successor emission
  failure.
- The evidence distinguishes this route from idea 321 halfword publication and
  idea 319 stack-homed fused compare control flow.
- `todo.md` records the candidate artifacts, proof command, and
  classification.

## Step 2: Add Focused Empty Loop-Exit Coverage

Goal: create focused backend coverage for a reachable empty loop-exit successor
at the end of an RV64 function.

Actions:

- Add a focused fixture that branches to an empty loop-exit successor requiring
  a valid return/epilogue path.
- Assert emitted RV64 contains a label and terminating path for the reachable
  empty successor, not fallthrough into unrelated text or data.
- Include runtime proof so a fallthrough into the next section cannot be hidden
  by assembly/link success.
- Keep the fixture independent of `src/00143.c`, fixed block names, and linked
  section names.

Completion checks:

- Focused dump/codegen/runtime coverage captures the empty-exit successor
  failure or uses the repo's expected-repair convention.
- Existing i16 halfword and stack-homed fused compare focused tests remain
  green.
- The test proves empty successor termination rather than another local-array
  or compare repair.

## Step 3: Repair Empty Successor Exit Emission

Goal: make reachable empty end-of-function successor blocks emit or share a
valid function exit path.

Actions:

- Locate RV64 function/block emission logic responsible for labels with no
  body instructions.
- Use prepared/function control-flow and return/epilogue facts to decide when
  an empty successor must emit a return path, jump to an epilogue, or share an
  existing exit block.
- Avoid special cases for block names, source filenames, or linked section
  layout.
- Re-run focused codegen/runtime coverage plus neighbor tests from ideas 319
  and 321.

Completion checks:

- The focused empty-exit successor test passes.
- The repair does not introduce traps or unsupported classifications for a
  reachable supported-path successor.
- Existing halfword local-array and stack-homed fused compare coverage remains
  green.

## Step 4: Reprobe Candidate and Classify Residuals

Goal: verify whether `src/00143.c` now completes or exposes a separate
mechanism after the empty-exit successor advances.

Actions:

- Reprobe `src/00143.c` through emit, link, and qemu after the focused repair.
- Inspect the emitted function end and linked runtime behavior.
- If qemu still fails, classify the first new failure with emitted-code
  evidence and decide whether it belongs outside idea 322.

Completion checks:

- `src/00143.c` either emits, links, and runs under qemu, or has a concrete
  out-of-scope residual classification.
- Generated assembly no longer defines a reachable empty end label that falls
  through into the next linked section.
- `todo.md` records candidate status and proof artifacts.

## Step 5: Close Classification

Goal: decide whether idea 322 is complete under its source acceptance criteria.

Actions:

- Re-run focused backend proof and candidate probe.
- Compare against existing backend regression logs or generate a matching close
  guard if closure is proposed.
- Create a separate follow-up idea only for a newly exposed residual outside
  this source idea.

Completion checks:

- Focused empty loop-exit successor coverage is green.
- `src/00143.c` is either `supported-linked-pass` or has a concrete
  out-of-scope residual classification.
- Close-time regression guard evidence is ready for plan-owner review.
