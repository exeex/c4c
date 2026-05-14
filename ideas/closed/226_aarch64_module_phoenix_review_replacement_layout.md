# AArch64 Module Phoenix Stage 2 Extraction Review And Replacement Layout

Status: Closed
Created: 2026-05-14
Closed: 2026-05-14
Parent Context: ideas/open/224_common_mir_container_and_target_printer_boundary.md
Requires: ideas/open/225_aarch64_module_phoenix_extract_legacy_evidence.md

## Intent

Review the stage-1 markdown evidence and define the replacement architecture
for AArch64 prepared-BIR-to-MIR lowering around direct MIR machine-node
production instead of the legacy module emitter and record accumulation shape.

Read this first before doing phoenix-stage work:
`.codex/skills/phoenix-rebuild/SKILL.md`.

## Stage In Sequence

Stage 2 of 4: extraction review, replacement layout, and stage-3 handoff.

## Produces

- One stage-2 review and replacement-layout document for the extracted
  AArch64 module scope.
- One explicit stage-2-to-stage-3 handoff document.

## Does Not Yet Own

- Per-file replacement `.cpp.md` / `.hpp.md` draft contents.
- Real implementation edits or dispatcher rewiring.
- Silent expansion beyond the module/MIR lowering surface unless the review
  explicitly says stage 1 must be repaired first.
- Test expectation changes or capability claims.

## Unlocks

Stage 3, which fills the exact replacement draft-file layout declared by this
stage and consumes the handoff as its intake contract.

## Scope Notes

- Review all accepted artifacts from stage 1:
  `src/backend/mir/aarch64/module/module.cpp.md`,
  `src/backend/mir/aarch64/module/module.hpp.md`, and
  `src/backend/mir/aarch64/module/module.md`.
- Reconstruct how the current module emitter works, including current
  dependencies on prepared BIR, prepared authority records, codegen records,
  public assembly, and API exposure.
- Judge whether the stage-1 extraction set itself needs correction,
  expansion, compression, splitting, merging, or reorganization before later
  stages trust it.
- Define a concrete replacement architecture layout for direct lowering from
  prepared BIR into MIR machine nodes.
- State whether the replacement layout addresses the parent 224 failure
  family: common MIR carrier confusion, flat target-local vectors, cached
  display strings, and target/common printer boundary drift.
- Declare the exact `.cpp.md` / `.hpp.md` draft-file layout that stage 3 must
  fill.
- The handoff document must state which extracted artifacts are trustworthy,
  which require correction first, which replacement draft files are mandatory,
  and what route constraints must be preserved.

## Stage 2 MIR/Printer Contract

- Replacement layout work must treat `prepare::PreparedBirModule` as the input
  to MIR lowering.
- Lowering must produce MIR nodes directly from that prepared module, not
  rebuild the legacy module-emitter record accumulation shape.
- Those MIR nodes must be sufficient for one shared, platform-independent
  `mir_printer` to scan once and emit `.s` text consumable by `gcc` / `as`.
- The shared `mir_printer` must not encode target assembly syntax. It should
  call target-owned print/render methods on AArch64 instructions, operands,
  registers, and other target forms.
- Instruction and operand nodes each need their own printable representation.
  Operands include immediates, registers, and any other target-specific operand
  forms required by the layout.
- Target-owned printing should be similar in spirit to a language object
  representation hook, but the C++ API must not be named `__repr__`.

## Boundaries

- Do not draft replacement component contents before the layout and handoff
  are accepted.
- Do not let current `module.cpp` helper boundaries dictate the new seams.
- Do not broaden into object encoding, linker behavior, x86, RISC-V, or broad
  new AArch64 instruction coverage.
- Do not claim stage-1 extraction is sufficient if required evidence is
  missing, unreviewable, or organized around the wrong surface.

## Completion Signal

This idea is complete only when the review/layout document explains the
current subsystem shape, identifies any required stage-1 extraction repairs,
defines the replacement layout, explicitly judges the parent 224 failure
family, declares the mandatory stage-3 draft artifact map, and the
stage-2-to-stage-3 handoff exists with enough concrete constraints that a
stage-3 executor can start without re-deriving the architecture.

## Completion Notes

Closed after Stage 2 acceptance verified:

- `src/backend/mir/aarch64/module/stage2_review_layout.md` exists and covers
  current subsystem shape, extraction quality/no-repair status, replacement
  layout, parent 224 judgment, and the Stage 3 draft artifact map.
- `src/backend/mir/aarch64/module/stage2_to_stage3_handoff.md` exists and
  matches the layout, mandatory draft map, no-repair evidence status, and
  route constraints.
- Stage 2 did not authorize implementation edits, build rewiring, test
  expectation changes, or replacement draft contents.
- `src/backend/mir/aarch64/module/module.cpp` remains compiled legacy evidence
  until replacement implementation plus build wiring exist in a later stage.

## Reviewer Reject Signals

Reject the route if the review rubber-stamps weak extraction, preserves the
legacy module emitter under new names, avoids the direct prepared-BIR-to-MIR
machine-node lowering decision, fails to name mandatory stage-3 draft files,
ignores parent 224 boundary failures, claims progress through expectation
rewrites or unsupported downgrades, or adds broad unrelated architecture work
without an explicit extraction-repair finding.
