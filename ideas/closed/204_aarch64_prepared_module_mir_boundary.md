# AArch64 Prepared-Module MIR Boundary

Status: Closed
Created: 2026-05-13
Closed: 2026-05-13

Depends On:
- `ideas/open/203_aarch64_markdown_first_backend_reconstruction.md`

## Goal

Define the first small code-oriented AArch64 follow-up after the
markdown-first reconstruction: a target-local MIR boundary that accepts
`c4c::backend::prepare::PreparedBirModule` and proves the prepared-module
handoff without starting instruction selection or assembly text emission.

The implementation should introduce enough AArch64-owned structure to make the
backend boundary reviewable and compilable: module, function, block, operand,
register, frame, branch, call, move, and data side-table skeletons keyed by
structured prepared ids.

## Why This Idea Exists

The AArch64 markdown-first gap ledger concludes that backend implementation
must not proceed directly to instruction selection or assembly text emission.
Most required shared facts already exist on `PreparedBirModule`, but AArch64
does not yet have a target-local MIR module, typed operands, register surface,
frame records, branch records, call records, move records, or data/object
side-table model that can consume those facts.

This idea creates that boundary first so later lowering work has a typed target
contract instead of reviving old rendered-name, LIR-text, or assembly-string
routes.

## In Scope

- Add an AArch64 prepared-module handoff that accepts
  `PreparedBirModule` as the target-local lowering input.
- Gate the handoff on an AArch64 `TargetProfile` and the AAPCS64 backend ABI.
- Reject non-AArch64 prepared modules before target MIR construction begins.
- Define structured-id keyed skeletons for:
  - AArch64 MIR module, function, and block records
  - operands retaining prepared value, value-name, type, symbol, string, and
    frame-slot identity where available
  - target register class and physical-register references distinct from
    semantic value identity
  - frame, stack-slot, dynamic-stack, and callee-save records sourced from
    prepared frame and stack plans
  - branch and compare records sourced from prepared control-flow facts
  - call records sourced from prepared call plans
  - move, copy, spill, reload, and ABI-binding records sourced from prepared
    value-location, regalloc, storage-plan, and parallel-copy facts
  - data/object side-table records for globals, strings, symbol visibility,
    TLS, constants, initializers, and later relocation needs
- Keep the skeleton small and compile-proven, with focused tests that exercise
  the handoff, target-profile/AAPCS64 gate, and representative structured-id
  preservation.
- Document any target-local fields that are intentionally placeholders for
  later instruction selection, assembler/object, or ABI-detail ideas.

## Out Of Scope

- AArch64 instruction selection.
- Assembly text emission or an assembly printer as the first rebuilt lowering
  API.
- Built-in assembler, object emitter, linker, or final executable production.
- Rendered-name recovery, printed-BIR parsing, legacy LIR text parsing,
  assembly-string parsing, parser operand recovery, or old AArch64 markdown
  examples as semantic input.
- Memory lowering that depends on volatile or address-space facts not yet
  carried by shared preparation.
- Fixing memory volatility or non-default address-space preservation inside
  AArch64 target code.
- NEON, broad vector coverage, inline assembly, f128/i128 special lowering, or
  broad target ABI completion beyond the boundary fields needed for the MIR
  skeleton.

If memory lowering is selected later, first split and solve the shared
prepared-carrier gap for memory volatility and address-space facts. The AArch64
MIR boundary may reserve fields for those facts, but it must not recover them
from rendered forms or silently assume defaults for a feature that requires
them.

## Acceptance Criteria

- The AArch64 backend has a compile-proven prepared-module entry or handoff
  surface that consumes `PreparedBirModule`.
- The entry rejects unsupported target profiles and requires the AAPCS64 ABI
  for the AArch64 route.
- The target-local MIR skeleton includes module/function/block and side-table
  records keyed by structured ids, not display strings.
- Operand, register, frame, branch, call, move, and data records preserve the
  relevant prepared identities needed by later lowering.
- The implementation does not emit assembly text, select concrete
  instructions, assemble objects, or link binaries.
- Focused tests or compile checks prove the handoff, gate behavior, and at
  least representative structured-id preservation.
- Any deferred fields are explicitly documented as later target MIR,
  target-ABI, instruction-selection, assembler/object, or shared-preparation
  work.

## Completion Notes

Closed after the active runbook completed Step 7 and the route review reported
no blockers. The implementation added the prepared-module AArch64 handoff gate,
structured-id keyed target-local module/function/block records, operand and
register identity records, descriptive frame/control/call/move records, and
data/object side-table records without instruction selection, assembly text
emission, assembler/object output, linker integration, or rendered-name
recovery.

Final proof used the focused AArch64 prepared-boundary tests plus the relevant
backend prepare tests:

```sh
cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_prepared_(handoff_gate|module_identity|operand_identity|frame_control|data_identity)|backend_prepare_)' >> test_after.log 2>&1
```

Deferred work remains future-scope only: richer target MIR instruction records,
deeper AAPCS64 ABI ownership, instruction selection, assembler/object/linker
work, and shared-preparation carriers for facts such as memory
volatility/address-space and register-bank identity.

## Reviewer Reject Signals

- The slice adds instruction selection, assembly text emission, an assembler,
  object writer, linker, or executable production and claims that as boundary
  progress.
- The implementation accepts raw `bir::Module` as the AArch64 target-local
  lowering input instead of requiring a prepared-module handoff.
- The target-profile or AAPCS64 gate is absent, lax, or tested only by a named
  testcase shortcut.
- Semantic identity is recovered from rendered names, printed BIR, LIR text,
  assembly strings, parser operands, or markdown examples instead of structured
  prepared ids.
- Memory lowering proceeds without first adding the missing shared prepared
  carriers for volatility or non-default address spaces when those facts are
  required.
- Tests are weakened, skipped, or reclassified to make the skeleton appear
  accepted.
- The change is classification-only, helper-renaming-only, or expectation-only
  churn claimed as backend capability progress.
- A broad rewrite of AArch64 instruction selection, target ABI, assembler,
  binary utilities, or linker code obscures whether the prepared-module MIR
  boundary itself works.
- The old failure mode remains behind a new abstraction name: target code still
  depends on text-shaped operands or legacy emitter state for semantic facts.
