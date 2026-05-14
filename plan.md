# AArch64 Structured ASM Encoder Linker Contract Runbook

Status: Active
Source Idea: ideas/open/218_aarch64_structured_asm_encoder_linker_contract.md

## Purpose

Clarify and enforce the internal AArch64 post-machine-node contract for any
future compile-through route that goes past terminal assembly text.

## Goal

Document and shape the backend contract so internal encoder, object, and linker
work consumes structured machine instruction nodes or deliberately lowered
structured asm/encoding records, never `.s` text emitted by
`src/backend/mir/aarch64/codegen/machine_printer.cpp`.

## Core Rule

`machine_printer.cpp` is a terminal text renderer for `--codegen asm`, analogous
to the LIR printer. Any internal assembler, encoder, object writer, or linker
route must preserve structured semantic payload instead of reparsing printed
assembly or accepting external `.ll`/`.s` as the normal backend input.

## Read First

- `ideas/open/218_aarch64_structured_asm_encoder_linker_contract.md`
- `src/backend/mir/aarch64/MACHINE_INSTRUCTION_NODE_CONTRACT.md`
- `src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md`
- `src/backend/mir/aarch64/BACKEND_ENTRY_CONTRACT.md`
- `src/backend/mir/aarch64/BINARY_UTILS_CONTRACT.md`
- `src/backend/mir/aarch64/assembler/mod.md`
- `src/backend/mir/aarch64/assembler/parser.md`
- `src/backend/mir/aarch64/assembler/elf_writer.md`
- `src/backend/mir/aarch64/assembler/encoder/mod.md`
- `src/backend/mir/aarch64/linker/mod.md`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`

## Current Targets

- AArch64 docs and contract markdown that describe codegen, assembler,
  parser, encoder, object, and linker boundaries.
- AArch64 assembler/linker headers only if a live contract surface must expose
  the structured boundary.
- Focused guard tests or compile checks only if an existing path can
  accidentally treat printed `.s` as internal backend input.

## Non-Goals

- Do not refactor `machine_printer.cpp` merely because it renders strings.
- Do not add an assembly token arena only for terminal printer output.
- Do not implement a full assembler, parser, encoder, ELF writer, object
  writer, linker, or binary emitter.
- Do not accept external `.ll` or `.s` as a supported backend input language.
- Do not remove or weaken the idea 216 external `.c -> .s -> clang/as -> run`
  smoke path.
- Do not expand instruction selection, printer opcode coverage, or encoder
  opcode coverage as part of this contract work.

## Working Model

- Terminal route:
  `machine instruction nodes -> machine_printer.cpp -> .s text -> external clang/as`.
- Rejected internal route:
  `machine instruction nodes -> .s text -> internal assembler parser -> encoder/object`.
- Accepted internal route:
  `machine instruction nodes -> structured asm/encoding operator records -> object records -> linker records -> binary output`.
- Prepared facts remain upstream authority for value homes, live ranges,
  interference, moves, spill/reload placement, and call preservation/clobber
  policy.
- The structured asm/encoding layer adds target-machine effects that only
  exist after instruction selection, including implicit register uses/defs,
  flags, subregister width effects, selected opcode clobbers, scratch ranges,
  and section/relocation/object ownership.

## Execution Rules

- Prefer docs/contracts for this plan. Touch implementation only when a live
  contract surface cannot express the accepted boundary without code.
- Preserve `--codegen asm` behavior and the terminal printer contract.
- Treat old parser/encoder markdown as historical reference unless this plan
  explicitly upgrades contract wording.
- Keep record shapes purpose-oriented. Do not replace the contract with one
  oversized operand struct plus broad enum dispatch.
- Centralize future enum spellings through mapping helpers in the accepted
  contract; do not scatter spelling switches through lowering or printer
  descriptions.
- Reject any route that claims progress by parsing `machine_printer.cpp`
  output back into semantics.

## Step 1: Audit AArch64 Boundary Wording

Goal: find every live or markdown AArch64 surface that could imply printed
assembly is an internal semantic bridge.

Primary targets:

- `src/backend/mir/aarch64/MACHINE_INSTRUCTION_NODE_CONTRACT.md`
- `src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md`
- `src/backend/mir/aarch64/BACKEND_ENTRY_CONTRACT.md`
- `src/backend/mir/aarch64/BINARY_UTILS_CONTRACT.md`
- `src/backend/mir/aarch64/assembler/*.md`
- `src/backend/mir/aarch64/assembler/encoder/*.md`
- `src/backend/mir/aarch64/linker/*.md`
- `src/backend/mir/aarch64/codegen/*.md`
- `src/backend/mir/aarch64/assembler/*.hpp`
- `src/backend/mir/aarch64/linker/*.hpp`
- `src/backend/mir/aarch64/codegen/emit.hpp`

Actions:

- Inspect references to assembler parser input, encoder input, object input,
  linker input, `machine_printer.cpp`, `.s` text, external `.s`, and external
  `.ll`.
- Classify each reference as already-safe, historical-only, ambiguous, or
  contract-breaking.
- Record the audit summary in `todo.md`; do not edit the source idea for
  routine findings.

Completion check:

- `todo.md` identifies the docs/headers that need edits and any surfaces that
  are already safe.

## Step 2: Mark Terminal Printer And Input Boundaries

Goal: update relevant contracts so terminal text output and internal structured
input are separated explicitly.

Actions:

- State that `machine_printer.cpp` is the terminal renderer for `--codegen asm`
  and is analogous to the LIR printer.
- State that internal compile-through routes consume machine instruction nodes
  or lower structured asm/encoding records, not printed `.s`.
- State that external `.ll` and `.s` may be fixtures or external-toolchain
  smoke outputs, but are not the normal input to backend codegen, encoder,
  object writer, or linker.
- Preserve the idea 216 external assembly smoke path as valid terminal-output
  validation.

Completion check:

- A reviewer can trace from machine nodes to terminal `.s` output and to the
  separate future structured internal route without ambiguity.

## Step 3: Define Structured ASM/Encoding Record Surface

Goal: document the minimum future structured record surface without
implementing the full encoder/object/linker.

Actions:

- Define the accepted record families: module or translation unit, section,
  label, operator/instruction, directive, data, typed operands, and relocation
  needs.
- Include the concrete register operand decomposition from the source idea:
  `AsmRegisterRef`, `AsmRegisterUseKind`, `AsmRegisterUse`,
  `AsmValueProvenance`, `AsmAllocationProvenance`, and
  `AsmRegisterOperand`.
- Preserve the separation between physical register reference, register use,
  value provenance, allocation provenance, and clobber/effect metadata.
- Identify typed operand, register, immediate, symbol id, link-name id,
  relocation, section/data ownership, branch/data reference, volatility,
  address-space, side-effect, clobber, and provenance facts that must survive
  toward object/linker work.

Completion check:

- The accepted contract describes readable class/struct surfaces, not only a
  broad enum plus one catch-all operand payload.

## Step 4: Tie Structured Records To Prepared And Machine-Level Facts

Goal: make upstream authority and post-selection additions explicit.

Actions:

- Name prepared facts that structured asm/encoding must consume:
  `PreparedLiveness`, `PreparedLiveInterval`, `PreparedRegalloc`,
  `PreparedRegallocValue`, `PreparedInterferenceEdge`,
  `PreparedMoveResolution`, `PreparedSpillReloadOp`,
  `PreparedValueLocations`, `PreparedCallPreservedValue`, and
  `PreparedClobberedRegister`.
- State which facts remain prepared authority and which facts must be added
  after instruction selection.
- Ensure machine instruction nodes remain the semantic boundary after target
  MIR; do not weaken them into printer-only artifacts.

Completion check:

- The contract explains how prepared lifecycle facts and machine-node effects
  both survive into future structured encoding/object records.

## Step 5: Centralize Enum Spelling Contract

Goal: document where future printer/diagnostic spelling for structured
asm/encoding enums belongs.

Actions:

- Require enum-to-string mapping functions for every new enum kind introduced
  for sections, labels, directives, operator/opcode categories, operand kinds,
  register use kinds, relocation kinds, and record surfaces.
- State that terminal printers and diagnostics should call these mappings
  instead of scattering spelling switches through lowering code.
- If a live enum is introduced or extended by this plan, add or update the
  matching mapping function and compile proof.

Completion check:

- A future executor knows that enum spelling is centralized as part of the
  structured record contract.

## Step 6: Add Focused Guards If A Live Risk Exists

Goal: prove that existing live paths do not accidentally depend on reparsing
terminal `.s` as backend semantics.

Actions:

- Inspect whether any live AArch64 path routes `machine_printer.cpp` output
  into assembler/parser/encoder/object/linker code.
- If no live route exists, record that no guard test is needed for this plan.
- If a live accidental route exists, add the narrowest compile or guard test
  that prevents printed `.s` from becoming internal semantic input.

Completion check:

- Either a focused guard exists, or `todo.md` records why the current code has
  no live risk requiring one.

## Step 7: Validate And Handoff

Goal: leave a small, reviewable contract slice with proof appropriate to the
files changed.

Actions:

- For docs-only changes, run a spelling/search sanity check over edited
  contract files and record the command.
- For header or code changes, run build or compile proof selected by the
  supervisor/executor packet.
- Check that no root-level noncanonical `.log` files were left behind.
- Update `todo.md` with proof, remaining risks, and suggested next work.

Completion check:

- Acceptance criteria from the source idea are either satisfied or remaining
  work is explicit in `todo.md`, and lifecycle invariants still point to this
  source idea.
