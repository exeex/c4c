# Prealloc Inline Asm Memory Effect Metadata Contract

## Goal

Make prealloc inline-asm stack placement consequences consume structured BIR
memory/address metadata whenever available, and document any remaining
unstructured conservative summary path.

## Why This Exists

The memory boundary audit classified inline asm and intrinsic memory operands
as BIR semantic facts when represented by `InlineAsmOperandMetadata` or
`IntrinsicOperation` memory/address metadata. Prealloc owns the placement
consequences, such as conservative home slots and target-facing operand
storage. The unresolved gap is where inline-asm summaries force placement
decisions from raw use shape rather than structured `MemoryAddress` or operand
metadata.

This should remain a narrow contract check: conservative placement for inline
asm may be correct, but the source of the memory-effect fact must be explicit.

## In Scope

- Trace `src/backend/prealloc/stack_layout/inline_asm.cpp` and related
  stack-layout hint consumers for inline-asm memory/address effects.
- Prefer structured BIR inline-asm memory/address metadata where present.
- Document any retained raw or conservative summary path as a named
  compatibility/safety path with clear limits.
- Add focused proof for an inline-asm memory operand that affects stack object
  publication or home-slot retention.

## Out Of Scope

- Rewriting inline asm parsing or target asm constraint lowering broadly.
- Moving stack placement, register homes, or operand storage decisions into
  BIR.
- Changing unrelated intrinsic memory lowering unless it shares the same
  metadata contract.
- Weakening inline-asm expectations to avoid modeling memory effects.

## Acceptance Criteria

- Inline-asm placement summaries consume structured memory/address metadata
  when that metadata exists.
- Any remaining unstructured summary is documented as conservative placement
  policy rather than BIR memory provenance authority.
- Proof demonstrates that a structured inline-asm memory operand drives the
  intended prealloc placement consequence.

## Reviewer Reject Signals

- The change keeps raw instruction-shape scans as the undocumented semantic
  authority for inline-asm memory effects.
- It claims progress by weakening inline-asm memory expectations or marking
  cases unsupported.
- It moves target placement or storage-home decisions into BIR.
- Proof covers only an asm string spelling while structured metadata consumers
  remain unexamined.
- The implementation broadens into unrelated inline-asm or intrinsic lowering
  redesign.

## Close Note

Closed on 2026-06-03.

The source idea is complete. Inline-asm stack-layout placement consequences now
consume structured inline-asm memory/address operand metadata where available:
`MemoryInput.memory_address` and `AddressInput.address` feed the existing
prealloc `MemoryAddress` pointer-root path. Structured
`MemoryAddress::PointerValue` roots mark the referenced local stack object
`address_exposed`, keep `requires_home_slot`, and retain frame-slot storage
with the original object layout.

The retained unstructured path is conservative placement policy only.
Side-effecting inline asm, including `"memory"` clobber cases, may reinforce
home slots for objects already address-exposed by other facts, but it is not
object-specific BIR memory provenance authority. Missing structured
memory/address metadata with a side-effecting inline asm and `"memory"` clobber
does not create object-specific stack placement facts from an unrooted call
argument.

Placement remains in prealloc stack layout. The route did not move stack
placement, register homes, operand storage, parser behavior, BIR lowering,
prepared inline-asm carrier validation, MIR, AArch64 lowering, target operand
selection, or intrinsic behavior into this contract.

Proof status: final backend validation passed with `169/169` backend tests, and
the close-time backend regression guard passed with `169/169` before and after,
no new failures, and no resolved failures. Coverage includes structured
`MemoryInput.memory_address`, structured `AddressInput.address`, the intended
local stack placement consequence, and the bounded fail-closed conservative
summary behavior.
