Status: Active
Source Idea Path: ideas/open/83_aarch64_local_helper_duplication_tail_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Local Helper Duplication

# Current Packet

## Just Finished

Step 1 inventory completed for idea 83. Repeated helper shapes found:

- Register view selection: `scalar_fp_register_view(bir::TypeKind)` and
  `scalar_storage_register_view(bir::TypeKind)` repeat in `globals.cpp`,
  `atomics.cpp`, `cast_ops.cpp`, and `memory.cpp`; `memory.cpp` and existing
  `fp_value_materialization.cpp` also need the register-reference overload for
  FP/SIMD views. Classification: fold the simple type-to-view helpers into an
  existing AArch64-local utility only if the owner already exposes scalar view
  authority; keep the FP-reference overload local to owners that materialize
  FP values.
- Prepared storage lookup: `find_storage_plan_value(storage_plan, value_id)`
  repeats in `globals.cpp`, `atomics.cpp`, `cast_ops.cpp`, and `memory.cpp`.
  `instruction.cpp` already has the same private utility for instruction-record
  builders, but it is not public. Classification: fold into an existing
  AArch64-local utility only through an owner already used by these record
  builders; otherwise leave owner-local for the first packet.
- Register class conversion: `register_class_from_bank` repeats in all four
  target owners and is also present in existing local utility-style code such
  as `instruction.cpp`, `alu.cpp`, `calls.cpp`, `intrinsics.cpp`, and
  `effects.cpp`. Classification: fold into an existing AArch64-local utility
  after the first owner-focused packet proves the smaller register-operand
  shape; no new prealloc authority.
- Register spelling and occupied registers: `register_display_name`,
  `occupied_register_views`, and `occupied_register_references` repeat in
  `globals.cpp`, `atomics.cpp`, and `memory.cpp`; `cast_ops.cpp` has the
  narrower spelling helper that intentionally omits `V`, `Q`, and `Sp`.
  Classification: narrow owner-local helper first in `globals.cpp` by using
  existing `abi::register_name` for occupied names, then consider repeating the
  same pattern in `atomics.cpp` and `memory.cpp`. Keep `cast_ops.cpp` local
  until cast-specific view restrictions are reviewed.
- Prepared register operand creation: `make_prepared_register_operand(home,
  storage, type, role)` repeats in `globals.cpp`, `atomics.cpp`, and
  `memory.cpp`; `cast_ops.cpp` is similar but requires a non-empty display name
  and is paired with `register_spelling_matches_storage_view`. Classification:
  fold same-shaped `globals`/`atomics`/`memory` logic only after a smaller
  owner-local cleanup; keep cast variant owner-local for now.
- Prepared value ID and named value lookup: `prepared_named_value_id` repeats
  in `globals.cpp`, `cast_ops.cpp`, `fp_value_materialization.cpp`, and memory
  uses several narrower `named_value_id`/`indexed_value_home_id` helpers with
  ambiguity checks. Classification: keep owner-local; the memory ambiguity
  behavior and FP producer lookup rules are semantic, not just helper shape.
- Frame slot and storage-plan helpers: `memory.cpp` owns
  `make_frame_slot_operand_from_stack_slot`, `resolve_frame_slot_memory_offset`,
  `apply_stack_layout_to_memory_record`, and pointer/local-address identity
  helpers. `prepared_value_home_materialization.cpp` already owns narrow
  prepared home publication to a register. Classification: keep memory helpers
  owner-local unless a future memory-only packet can delegate publication to
  `prepared_value_home_materialization.*` without moving addressing policy.
- Diagnostics and record-error wrappers: each owner has family-specific
  wrappers (`append_address_materialization_diagnostic`,
  `append_memory_diagnostic`, `*_record_error`, `*_error_message`) tied to
  instruction family and diagnostic enum. Classification: keep owner-local.
- Occupied-register and scratch selection: `atomics.cpp` owns
  `record_uses_gp_register` and `next_reserved_gp_scratch_operand` for atomic
  loop records; `memory.cpp` owns `make_load_result_stack_publication_scratch`;
  `alu.cpp` already owns broader scalar scratch utilities. Classification:
  keep atomic and memory scratch selection owner-local unless a later packet
  finds the exact same `RegisterOperand` population shape can use a shared
  AArch64-local helper without changing scratch policy.
- Existing utility owners: `operands.*` owns prepared home/storage decode into
  generic MIR operands and diagnostics; `prepared_value_home_materialization.*`
  owns emitting prepared homes/publication plans into registers; and
  `fp_value_materialization.*` owns FP immediate/value materialization plus
  FP producer lookups. Classification: use these only for their current
  authorities; do not expand them into global/atomic/memory lowering policy.
- Out of scope/no-touch: BIR or prealloc lookup semantics, storage-plan
  authority, regalloc/value-home decode, address legality, global relocation
  policy, atomic ordering/loop construction, scalar/FP cast semantics,
  diagnostic contract weakening, expectation rewrites, and testcase-shaped
  shortcuts.

## Suggested Next

First implementation packet: Step 2 globals owner-local cleanup. Exact files:
`src/backend/mir/aarch64/codegen/globals.cpp` only, plus `todo.md` for the
executor handoff. Replace the globals-local `register_display_name` table with
existing `abi::register_name` inside `occupied_register_views`, preserving
empty-name behavior and all address materialization record semantics. Do not
touch `atomics.cpp`, `cast_ops.cpp`, `memory.cpp`, headers, tests, or shared
utility APIs in this first packet.

Proof command for that code-changing packet:
`cmake --build build -j --target c4c && ctest --test-dir build -R 'backend_(aarch64_target_instruction_records|codegen_route_aarch64_got_load_global_prepared_memory|codegen_route_aarch64_store_global_stack_publication|codegen_route_aarch64_global_function_pointer_table_selected_indirect_call)' --output-on-failure`

## Watchouts

The globals packet is intentionally smaller than the full helper fold. A shared
helper extraction across `globals`, `atomics`, and `memory` should wait until
one owner proves the `abi::register_name` substitution does not change occupied
register metadata. Cast spelling is narrower than the other owners and should
not be folded with them in the first pass.

## Proof

Inventory-only slice; no build required and no `test_after.log` produced by
contract. Validation: `git diff --check`.
