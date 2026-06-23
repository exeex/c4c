# RV64 Duff Fallthrough Pointer Update Producers Plan

Status: Active
Source Idea: ideas/open/324_rv64_duff_fallthrough_pointer_update_producers.md

## Purpose

Repair semantic/prepared producer publication for pointer update values in
Duff's-device-style fallthrough copy blocks where later cases store next
`from`/`to` values into pointer locals without corresponding `bir.add ptr`
producer facts.

## Goal

Ensure later Duff fallthrough pointer updates have semantic/prepared producer
facts before RV64 consumes them, without matching `src/00143.c`, Duff block
names, `%t39`, `%t42`, pointer local names, fixed stack offsets, or fixed array
sizes.

## Core Rule

This route owns missing producer facts for later fallthrough pointer update
values. Do not repair it by teaching RV64 to guess from uninitialized stack
homes, and do not reopen already-valid loop-carried pointer consumption from
idea 323 unless fresh evidence proves a regression.

## Read First

- `ideas/open/324_rv64_duff_fallthrough_pointer_update_producers.md`
- Idea 323 Step 5 artifacts under
  `build/rv64_c_testsuite_probe_latest/triage_323_step5/`
- Focused loop-carried pointer post-increment coverage from idea 323, so this
  route distinguishes missing producer facts from RV64 consumption of facts
  that already exist.

## Scope

- Semantic/prepared producer facts for pointer update values in later Duff
  fallthrough copy blocks.
- Pointer local publication for `%lv.from`/`%lv.to`-style updates when the next
  pointer value should be produced from the current pointer value.
- Focused backend coverage that distinguishes missing producer facts from RV64
  consumption of producer facts that already exist.
- Candidate reprobe for `src/00143.c` after ideas 322 and 323 have advanced.

## Non-Goals

- RV64 consumption of valid loop-carried pointer post-increment facts from idea
  323.
- Empty loop-exit successor emission from idea 322.
- I16 local-array select/store publication from idea 321.
- Stack-homed fused compare control flow from idea 319.
- Nested select-chain/store-source publication for `src/00144.c` from idea
  320.
- Aggregate/byval repair, function-pointer repair, external-call policy, or
  broad switch/control-flow rewrites beyond Duff fallthrough pointer update
  producer publication.

## Working Model

After idea 323, RV64 can consume valid pointer-add producer facts when they
exist. `src/00143.c` still segfaults because later fallthrough block `block_9`
stores `%t39` and `%t42` into pointer locals, but BIR lacks corresponding
`bir.add ptr` producers. Prepared-BIR therefore assigns stack homes with
`source_producer=unknown`, and emitted RV64 loads uninitialized stack homes and
publishes those values as pointer locals.

## Execution Rules

- Create or preserve semantic/prepared pointer producer facts before RV64
  consumption.
- Do not match `src/00143.c`, Duff's-device block names, `%t39`, `%t42`,
  `%lv.from`, `%lv.to`, fixed stack offsets, or fixed array sizes.
- Do not avoid later Duff fallthrough entries such as `count % 8 == 7` in
  focused fixtures just to make proof pass.
- Keep runtime proof on the supported path; do not weaken qemu coverage or mark
  the candidate unsupported.
- Validation ladder for implementation steps: build, focused backend proof,
  candidate reprobe, then backend regression guard when closure is proposed.

## Step 1: Normalize Duff Fallthrough Producer Evidence

Goal: reproduce the current `src/00143.c` segfault and identify the first
missing pointer update producer fact in a later Duff fallthrough block.

Actions:

- Reprobe `src/00143.c` through BIR, prepared-BIR, RV64 emit, clang link, and
  qemu.
- Confirm idea 323's first-block current-pointer post-increment path still
  advances.
- Inspect BIR/prepared-BIR around later fallthrough blocks such as `block_9`
  where pointer locals are updated from values with unknown producers.
- Record where next pointer values lack `bir.add ptr` or equivalent semantic
  producer facts and how emitted RV64 consumes their homes.

Completion checks:

- Fresh evidence identifies the first missing Duff fallthrough pointer update
  producer.
- The evidence distinguishes this route from idea 323 RV64 consumption of
  already-valid producers.
- `todo.md` records candidate artifacts, proof command, and classification.

## Step 2: Add Focused Duff Fallthrough Producer Coverage

Goal: create focused backend coverage for later fallthrough pointer update
blocks that need explicit producer facts before local publication.

Actions:

- Add a focused fixture with a later fallthrough entry where pointer locals are
  updated after the first copy block.
- Assert prepared facts strongly enough to catch `source_producer=unknown` on
  next pointer values that are immediately stored into pointer locals.
- Assert generated RV64 strongly enough to catch loads from uninitialized stack
  homes being published as pointer locals.
- Include runtime proof that fails when later fallthrough pointer updates use
  uninitialized or stale values.

Completion checks:

- Focused dump/codegen/runtime coverage captures the missing producer gap or
  uses the repo's expected-repair convention.
- The fixture does not depend on candidate filename, Duff block names, SSA
  names, fixed offsets, or fixed array sizes.
- Idea 323 focused coverage remains green.

## Step 3: Repair Semantic/Prepared Pointer Producers

Goal: ensure later Duff fallthrough pointer update values have semantic
producer facts before they are published into pointer locals.

Actions:

- Locate where BIR or prepared facts for fallthrough pointer updates are built
  or propagated.
- Create or preserve explicit pointer-add producer facts for next `from`/`to`
  values in later fallthrough copy blocks.
- Ensure prepared source-producer records point at those semantic producers
  rather than unknown stack homes.
- Avoid RV64-only synthesis and testcase-shaped fallback matching.

Completion checks:

- Focused prepared dump coverage no longer shows unknown source producers for
  immediately published next pointer values.
- The repair is not tied to `src/00143.c`, Duff's-device spelling, block names,
  SSA names, fixed offsets, or fixed array sizes.
- Existing loop-carried pointer consumption proof remains green.

## Step 4: Repair RV64 Publication Outcome

Goal: verify RV64 consumes the repaired producer facts and no longer publishes
uninitialized stack-home values as pointer locals.

Actions:

- Update RV64 only if needed to consume the repaired prepared producer facts.
- Run focused dump/codegen/runtime proof.
- Reprobe `src/00143.c` through emit, link, and qemu.
- If `src/00143.c` advances to a distinct residual, classify it rather than
  expanding this plan silently.

Completion checks:

- Focused runtime coverage passes without publishing unknown producer homes.
- `src/00143.c` either emits, links, and runs under qemu, or has a concrete
  out-of-scope residual classification.
- Generated RV64 no longer loads uninitialized stack homes for later Duff
  fallthrough pointer updates in focused proof cases.

## Step 5: Reprobe and Close Classification

Goal: decide whether idea 324 is complete under its source acceptance criteria.

Actions:

- Re-run focused backend proof and the `src/00143.c` candidate probe.
- Compare against existing backend regression logs or generate a matching close
  guard if closure is proposed.
- Create a separate follow-up idea only for a newly exposed residual outside
  this source idea.

Completion checks:

- Focused Duff fallthrough pointer update producer coverage is green.
- `src/00143.c` is either `supported-linked-pass` or has a concrete
  out-of-scope residual classification.
- Close-time regression guard evidence is ready for plan-owner review.
