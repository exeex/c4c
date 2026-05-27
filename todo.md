Status: Active
Source Idea Path: ideas/open/43_shared_prepared_typed_stack_source_authority.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Audit Typed Stack-Source Authority

# Current Packet

## Just Finished

Completed Step 1 audit of typed scalar concrete-offset `StackSlot -> Register`
prepared authority.

Existing shared facts:

- Source slot identity and concrete stack address: `PreparedValueHome` already
  carries `kind == StackSlot`, optional `slot_id`, `offset_bytes`,
  `size_bytes`, and `align_bytes`; `PreparedEdgePublication` preserves the
  `source_home` pointer and `source_home_kind`.
- Edge-publication source and destination scalar type: `PreparedEdgePublication`
  already carries full `bir::Value source_value` and `destination_value`, so
  `source_value.type` and `destination_value.type` are shared lookup facts.
- Source/destination value identity: `PreparedEdgePublication` carries
  `source_value_id`, `source_value_name`, `destination_value_id`, and
  `destination_value_name`.
- Destination home/storage authority: `PreparedEdgePublication` carries
  `destination_home`, `destination_home_kind`, and
  `destination_storage_kind`; the matched `PreparedMoveResolution` carries
  `destination_storage_kind`, optional `destination_register_name`, and optional
  `destination_register_placement`.
- Destination register bank/view authority exists upstream only when the move
  producer populated `destination_register_placement`; that placement carries
  `PreparedRegisterBank`, pool, slot index, and contiguous width. Generic
  `PreparedValueHome` also has optional `target_register_identity`, but normal
  regalloc value-home classification does not populate it.

Missing or unsafe facts:

- No shared prepared field currently records scalar signedness or extension
  policy for edge-publication stack loads. Same-width `I32 -> I32` and
  `I64 -> I64` do not need an extension decision, but subword and widening
  forms do.
- RISC-V `EdgePublicationMoveIntent` records source stack slot/offset/size and
  destination register spelling, but it does not record source scalar type,
  signedness/extension policy, or destination register bank/view.
- Current RISC-V stack-source register emission uses
  `has_untyped_integer_stack_source_register_policy()` plus byte size to choose
  `lw` or `ld`; this is an unsafe target-local hint for the typed-authority
  route because it is not backed by copied shared bank/type/extension facts in
  the intent.
- Raw destination register spelling, byte size, ids, offsets, and fixture names
  must remain non-authoritative. The existing F32, subword, widening, dynamic,
  and aggregate fail-closed tests correctly identify these unsafe inference
  routes.

Selected first typed scalar form for Step 2:

- Same-width `I32` concrete-offset `StackSlot(size_bytes=4) -> Register` where
  `publication.source_value.type == I32`,
  `publication.destination_value.type == I32`, destination storage is
  `Register`, and shared destination bank/view authority is explicitly `Gpr`
  via `publication.move->destination_register_placement` or a new shared helper
  that exposes equivalent prepared authority.
- Step 2 should add/copy target-neutral prepared authority for this selected
  form before RISC-V consumes it: source stack slot/offset/size, scalar type
  `I32`, no-extension/same-width policy, and destination `Gpr` bank/view.
  If the implementation cannot obtain `Gpr` from existing producer-populated
  `destination_register_placement`, the exact upstream producer gap is missing
  destination register placement/bank for edge-publication register moves.

## Suggested Next

Implement Step 2 by adding the minimal shared prepared carrier/query for the
selected same-width `I32` concrete-offset stack-source publication, including a
fail-closed unavailable state when destination `Gpr` bank/view or same-width
type authority is missing.

## Watchouts

The current RISC-V helper already emits `lw`/`ld` for same-width integer stack
sources, but that path is still size/type-shape driven inside RISC-V. Do not
accept Step 2/3 as progress unless the emitted path is authorized by shared
prepared facts copied into a shared record or intent field, and keep subword,
widening, F32, dynamic-address, and aggregate-width stack sources fail-closed.

## Proof

No build/test proof required for this audit-only packet. No code changed.
