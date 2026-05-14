# AArch64 Module Phoenix Stage 4 Drafts To Implementation

Status: Closed
Created: 2026-05-14
Parent Context: ideas/open/224_common_mir_container_and_target_printer_boundary.md
Requires: ideas/closed/227_aarch64_module_phoenix_replacement_drafts.md
Closed: 2026-05-14

## Intent

Convert the reviewed replacement drafts into real AArch64 module/MIR lowering
implementation, routing prepared BIR directly into MIR machine nodes through
the reviewed ownership seams.

Read this first before doing phoenix-stage work:
`.codex/skills/phoenix-rebuild/SKILL.md`.

## Stage In Sequence

Stage 4 of 4: staged implementation conversion, dispatcher rewiring, proof,
and legacy retirement.

## Produces

- Real `.cpp` / `.hpp` implementation matching the reviewed draft set.
- Dispatcher and ownership rewiring that routes behavior through the reviewed
  replacement seams.
- Explicit accounting of moved responsibilities and remaining legacy
  compatibility.
- Legacy deletion or retirement only after the new owner is live and proved.
  The current route has already removed the legacy AArch64 module emitter in
  `252dbc50c`, with a fresh skeleton added in `0fce192f4`; lifecycle execution
  must continue from that committed state by migrating stale tests and adding
  replacement seams, not by restoring the old broad record-pile owner.

## Does Not Yet Own

- Reopening the stage-2 layout without a lifecycle repair.
- Reinterpreting prepared authority semantics for frame, call, register
  allocation, spill/reload, storage plan, data, or control-flow facts.
- Broad instruction coverage unrelated to proving the migrated seam.
- Object encoding, ELF relocation emission, linker behavior, x86, or RISC-V.

## Unlocks

Closure review for the AArch64 module phoenix rebuild route and, if still
needed, a separate follow-up idea for any compatibility or target-printer work
left outside the rebuilt module/MIR lowering surface.

## Scope Notes

- Migrate in behavior-preserving slices rather than an all-at-once rewrite.
- Keep old live behavior available until the new dispatcher can choose the
  replacement path for the migrated seam where that legacy behavior still
  exists. For the already-deleted AArch64 module emitter, keep incomplete public
  assembly paths fail-closed until replacement lowering is proved rather than
  resurrecting the deleted record-pile emitter.
- Typical migration order is shared helpers, value/home or operand resolution,
  one coherent lowering family, dispatch rewiring, then deletion of dead
  compatibility code.
- After the fresh AArch64 skeleton, migrate stale legacy module-record compile
  tests before function traversal: replace/update/remove dependencies on
  deleted APIs such as `OperandRecord`, `FrameRecord`, broad `FunctionRecord`
  record piles, and `module.globals` legacy views with new-route tests for the
  common MIR carrier, skeleton construction, fail-closed
  assembly/no-machine-nodes behavior, and future traversal/operand contracts.
  This is not a test downgrade; do not restore the legacy module emitter or the
  old record pile to satisfy stale tests.
- Every migration packet must state what responsibility moved, what still
  remains in legacy code, and what proof covers the moved seam.
- Proof must cover each migrated capability family before completion is
  claimed.

## Boundaries

- Do not weaken tests, mark supported paths unsupported, or accept
  expectation-only progress.
- Do not add testcase-shaped shortcuts or named-case-only lowering.
- Do not silently keep the exact old module-emitter failure mode behind a new
  abstraction name.
- Do not delete compatibility code until the replacement owner is live and
  proved.

## Completion Signal

This idea is complete only when the reviewed draft set has been converted into
real implementation, the new ownership seams are actually in use, remaining
legacy code is explicitly classified as compatibility or follow-up scope,
direct prepared-BIR-to-MIR machine-node lowering is the active route for the
migrated surface, and proof shows the migrated capability families still work.

## Closure Ledger

Closed after Stage 4 implementation converted the reviewed AArch64 module
drafts into compiled module facade and codegen ownership surfaces. The active
route now sends prepared BIR through codegen traversal, dispatch, operands,
lowering families, emit orchestration, and derived compatibility projection
instead of restoring the deleted legacy module emitter or broad record-pile
owner.

Closure proof:

- full CTest proof in `test_after.log`: `3167/3167` passed
- regression guard comparison against `test_before.log`: no new failures and
  no pass-count loss; closure was accepted with an equal-pass-count lifecycle
  guard because this final slice is lifecycle-only

Residual scope is explicitly outside this idea: `codegen/machine_printer.*`
remains a temporary terminal compatibility printer until idea 224 replaces it
with the shared MIR printer boundary. The compatibility projection under
`codegen/compatibility_projection.*` is derived-only compatibility state and is
not semantic lowering authority.

## Reviewer Reject Signals

Reject the route if implementation diverges from reviewed drafts without a
lifecycle repair, skips staged proof, removes legacy code before the new owner
is live, weakens tests, adds named-case shortcuts, leaves direct lowering
unused behind dispatcher compatibility, or preserves the old module emitter
and record design as the real owner under replacement filenames.
