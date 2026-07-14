# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.25
Current Step Title: Publish i64 inline-asm output binding authority

## Just Finished

- Stopped Plan Step 7.24 without code changes and classified producer-specific
  implicit call-argument coercion as an exact separately owned blocker.
- Erasing or replacing the SExt source ID removes selected-global provenance,
  but the remaining exact i32-to-i64 SExt plus fixed i64 call-argument shape is
  also valid for other authoritative i32 producers. Inferring the source from
  that shape overclaims; declining the claim cannot reject the required
  missing or wrong selected-global source mutation.
- Truthful producer-specific coercion verification therefore requires a native
  coercion/source-purpose carrier. That design belongs to a separate future
  semantic-carrier initiative; producer/name/text inference is forbidden.
- Step 7 remains active: the accepted scalar inline-asm Output role/type/index
  contract has one carrier-ready i64 width neighbor that needs no source-purpose
  inference, input-count schema, or multi-result mapping.
- Builtin parity, scalar multi-output inline assembly, and inline-asm inputs
  retain their exact separately owned blockers and are not reopened here.

## Suggested Next

- Executor: complete Plan Step 7.25 for only PS's single output-only scalar i64
  `LirInlineAsmOp` route. Allocate the semantic output through `fresh_value`,
  store the exact ID in its native i64/Output/index-zero result binding, and
  preserve that ID through representation-preserving coercion into the later
  type-matched i64 Store.

## Watchouts

- Own exactly one non-explicit-register output-only i64 binding. Require one
  function-owned definition and exact agreement among native i64 type, Output
  role, output index zero, result collection position/count, operation return
  type, and the later Store type/use.
- Reuse the Step-7.21 output binding and ownership mechanism without changing
  its accepted i32 contract. Do not add a new width-purpose field or infer
  width/identity from rendered constraint, result, operand, or temporary text.
- Compatibility `result`, rendered operands, original assembly/constraint
  text, clobbers, and all other mirrors remain presentation/opaque. They may
  observe native authority but never create, repair, select, or validate it.
- Reachable verification must reject invalid/duplicate/missing output
  definitions; wrong role/index/count/position or type alternatives; unknown
  or cross-function Store uses; and any i32/i64 or binding-to-Store type
  conflict. Misleading displays with unchanged native facts must pass.
- Exclude accepted i32 Step 7.21, inputs, tied/read-write, multi-output, memory,
  address, immediate, clobber, explicit-register, floating/vector/aggregate
  bindings, `insn_r` semantics, opaque-text interpretation, compatibility-
  result authority, stack/local/object and body parameters, CFG, BIR receipt,
  parity, and all idea-741 contracts.
- This packet is carrier-ready only on the existing single-result inline-asm
  Output role/type/index carrier, `fresh_value`, representation-preserving
  Store edge, and ownership verifier. If i64 publication needs a new semantic
  carrier or any name/text-derived bridge, stop and return that exact blocker
  rather than widening Step 7.25.

## Proof

- Fresh `cmake --build --preset default`.
- Focused `ctest --test-dir build -R '^frontend_hir_tests$' --output-on-failure`
  with an output-only i64 identity/Store chain, misleading-display positives,
  and malformed definition, role/index/count/position, width, and Store-use
  cases.
- Preserve Step-7.21 i32 output behavior, neighboring inline-asm metadata/
  diagnostics, and accepted generic identity coverage unchanged; the
  supervisor owns matched regression logs and any broader/full proof.
- Run `git diff --check` before handoff.
