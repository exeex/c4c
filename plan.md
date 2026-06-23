# RV64 I16 Local Array Select Store Publication Plan

Status: Active
Source Idea: ideas/open/321_rv64_i16_local_array_select_store_publication.md

## Purpose

Repair RV64 prepared lowering for halfword local-array select/store publication
when an i16 local-array body follows an already-emitted stack-homed fused
compare loop branch.

## Goal

Make the RV64 local-array select/store path publish and consume the facts
needed to emit the halfword body exposed by `src/00143.c`, without matching the
candidate filename, block labels, SSA names, or observed stack offsets.

## Core Rule

Treat stack-homed fused compare control flow as already out of scope. This
route starts after the first loop condition emits; progress must repair the
later i16 local-array select/store body or classify a different first failure
with concrete emitted-code evidence.

## Read First

- `ideas/open/321_rv64_i16_local_array_select_store_publication.md`
- Idea 319 Step 5 evidence for `src/00143.c`, especially the reprobe showing
  the loop branch and false-successor jump emit before truncation inside
  `.Lmain_block_1`.
- Existing local-array, select publication, halfword load/store, and stack-homed
  fused compare focused backend tests, so the route separates this residual
  from already closed work.

## Scope

- RV64 prepared lowering for i16 local-array select/store publication.
- Halfword local-array stores reached after a stack-homed fused compare loop
  branch has already emitted.
- Destination/source publication facts for the `%t9.store0`-style local-array
  select/store family exposed by `src/00143.c`.
- Focused backend coverage that separates halfword local-array select/store
  body emission from stack-homed fused compare control flow.

## Non-Goals

- Stack-homed fused compare branch or loop-condition repair from idea 319.
- Nested select-chain/store-source publication for `src/00144.c` from idea 320.
- Pointer-to-pointer local frame-address materialization from idea 316.
- Aggregate/byval repair, function-pointer repair, external-call policy, or
  broad switch/control-flow work.

## Working Model

Fresh idea 319 evidence says `src/00143.c` emits the stack-homed fused compare
loop branch and jumps to the false successor, then truncates later inside the
first loop body before defining the false-successor label. Prepared body facts
point to the i32-to-i64 index cast followed by i16 local-array select/store
publication, including the `%t9.store0` family. This plan owns that later
halfword local-array body failure only.

## Execution Rules

- Use prepared local-array/select/store publication facts as the authority.
- Do not match `src/00143.c`, `.Lmain_block_1`, `.Lmain_block_2`,
  `%t9.store0`, fixed source spelling, or observed stack offsets.
- Do not claim progress by widening focused storage from i16/halfword to int
  unless the change is explicitly recorded as a reclassification.
- Keep idea 320's nested select-chain store-source path and idea 319's
  stack-homed fused compare control-flow path out of this route.
- Validation ladder for implementation steps: build, focused backend proof,
  candidate reprobe, then backend regression guard when closure is proposed.

## Step 1: Normalize I16 Local-Array Select/Store Evidence

Goal: reproduce the current `src/00143.c` residual and identify the first
i16 local-array select/store publication failure after the loop branch.

Actions:

- Reprobe `src/00143.c` through BIR, prepared-BIR, RV64 emit, clang link, and
  qemu when link succeeds.
- Confirm that stack-homed fused compare loop-control emission has already
  progressed before the first failure owned by this plan.
- Inspect prepared facts around the i32-to-i64 index cast and the
  `%t9.store0`-style halfword local-array select/store family.
- Record whether the emitted RV64 truncates before successor labels, emits a
  bad halfword access, fails link/runtime, or exposes a different mechanism.

Completion checks:

- Fresh evidence identifies the first local-array/select/store body failure.
- The evidence distinguishes this route from idea 319 control flow and idea 320
  nested store-source publication.
- `todo.md` records the candidate artifacts, proof command, and classification.

## Step 2: Add Focused I16 Local-Array Select/Store Coverage

Goal: create focused backend coverage for halfword local-array select/store
publication without depending on `src/00143.c`.

Actions:

- Add a focused fixture that reaches an i16 local-array select/store body after
  ordinary control flow has already selected the loop path.
- Assert prepared facts strongly enough to catch missing destination/source
  publication for the halfword local-array store.
- Assert generated RV64 strongly enough to catch truncation before required
  successor labels or return paths.
- Keep a nearby positive neighbor when useful so the test does not collapse
  into a filename-shaped reproduction.

Completion checks:

- Focused dump/codegen coverage captures the current halfword local-array
  publication gap or is marked with the repo's expected-repair convention.
- Existing stack-homed fused compare and nested store-source neighbors remain
  green.
- The fixture preserves i16/halfword storage instead of proving only a widened
  int-array case.

## Step 3: Repair Prepared Local-Array Select/Store Publication

Goal: make prepared facts publish the destination/source access needed for the
i16 local-array select/store body.

Actions:

- Locate where prepared local-array select/store publication records are built
  or propagated for halfword local stores.
- Preserve the relevant local-array destination and selected store source
  through casts/selects using existing prepared authority.
- Avoid RV64-only source scans or testcase-shaped fallback matching.
- Re-run focused prepared dump tests plus local-array/select neighbors.

Completion checks:

- Focused prepared records expose usable publication facts for the i16
  local-array select/store path.
- The repair is not tied to `src/00143.c`, `.Lmain_block_*`, `%t9.store0`, or
  fixed offsets.
- Previously closed 319 and 320 coverage remains green in the focused proof.

## Step 4: Repair RV64 Halfword Body Emission

Goal: ensure RV64 lowering consumes the repaired facts and emits complete code
for the halfword local-array select/store body.

Actions:

- Update RV64 lowering only where it consumes the prepared local-array
  select/store publication facts.
- Emit the halfword store/load/address sequence needed for the focused body
  without truncating before successor labels or `ret`.
- Run focused dump/codegen/runtime proof and reprobe `src/00143.c`.
- If `src/00143.c` advances to a distinct residual, classify it rather than
  expanding this plan silently.

Completion checks:

- Focused RV64 codegen/runtime coverage for the i16 local-array select/store
  body is green.
- `src/00143.c` either emits, links, and runs under qemu, or has a concrete
  out-of-scope residual classification after this body advances.
- No nested store-source, fused compare, ABI, external-call, or broad
  control-flow work is mixed into this step.

## Step 5: Reprobe and Close Classification

Goal: decide whether idea 321 is complete under its source acceptance criteria.

Actions:

- Re-run the focused backend proof and the `src/00143.c` candidate probe.
- Compare against existing backend regression logs or generate a matching close
  guard if closure is proposed.
- Create a separate follow-up idea only for a newly exposed residual outside
  this source idea.

Completion checks:

- Focused i16 local-array select/store publication coverage is green.
- `src/00143.c` is either `supported-linked-pass` or has a concrete
  out-of-scope residual classification.
- Close-time regression guard evidence is ready for plan-owner review.
