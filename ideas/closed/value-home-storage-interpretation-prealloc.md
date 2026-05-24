# Value Home and Storage Interpretation in Prealloc

## Intent

Move target-independent interpretation of Prepared value homes and storage
encodings out of AArch64 operands code and into prealloc-owned helper APIs.

## Why This Exists

The current AArch64 operand layer decodes Prepared homes, storage-plan values,
assignments, frame slots, immediates, symbols, and labels before mapping them to
target operands. The final target operand spelling belongs in AArch64, but the
meaning of a Prepared storage record should be owned by the layer that produced
or finalized that contract.

x86 has the same need when lowering prepared operands, and RISC-V will need it
if it adopts PreparedBirModule consumption.

## Current AArch64-Owned Responsibility

Current responsibility is concentrated in `src/backend/mir/aarch64/codegen`
operand helpers that decode generic Prepared value homes and storage encodings
before target-specific register-bank, addressing, immediate, symbol, label, or
frame-slot mapping.

## Proposed Destination Layer

`src/backend/prealloc` should provide storage and value-home interpretation
helpers. Targets should keep hooks or mapping functions for final register bank,
operand form, immediate legality, and target addressing constraints.

## x86/RISC-V Benefit

x86 prepared operand lowering can reuse target-neutral decoded storage/home
forms while keeping x86 register classes and operand encoding local. RISC-V
benefits later from the same storage interpretation boundary when it starts
consuming Prepared value homes.

## In Scope

- Separate policy-free Prepared home/storage decoding from AArch64 operand
  spelling.
- Add prealloc helper APIs that return target-neutral decoded storage/home
  decisions.
- Adapt AArch64 operands code to consume the helper and keep final AArch64
  operand construction local.
- Document which decoded forms x86 can reuse for its prepared operand lowering.
- Prove unchanged behavior across register, stack/frame-slot, immediate,
  symbol, and label forms where currently supported.

## Out of Scope

- Moving AArch64 operand classes, register names, immediate legality, memory
  addressing modes, or instruction selection into prealloc.
- Normalizing semantic BIR facts before prealloc.
- Changing value-home assignment algorithms.
- Weakening tests around unsupported operand forms.

## Proof Needed To Avoid Testcase Overfit

Acceptance needs tests or before/after logs covering multiple storage/home
forms, including at least one register-backed value, one frame or stack-backed
value, and one symbolic or label-like form if supported by current lowering.
The proof must show unchanged AArch64 output and must not rely on a single
known testcase.

The slice should include a concrete note showing how x86 prepared operand
lowering can reuse the target-neutral decoded form.

## Reviewer Reject Signals

Reject the route if it moves target register spelling, AArch64 addressing
legality, or final operand construction into prealloc. Reject expectation
downgrades, unsupported reclassification without explicit approval, or
case-shaped logic keyed to one storage spelling.

Reject if the new API only wraps the old AArch64 helper names without moving
ownership of Prepared storage interpretation, or if x86 would still need to
reimplement the same decoding from raw Prepared records.

## Completion Note

Closed after the active runbook moved policy-free Prepared value-home and
storage interpretation into prealloc-owned decoded helper APIs, adapted AArch64
to consume those decoded forms while retaining final target operand
construction locally, and exposed a concrete x86 prepared reuse path.

Validation covered register, frame/stack, immediate, symbol, missing-field,
unsupported, precedence, and fallback forms through backend tests including
`backend_prealloc_decoded_home_storage`,
`backend_aarch64_operand_resolution`, and
`backend_x86_prepared_decoded_home_storage`. No tests or expectations were
weakened or reclassified.
