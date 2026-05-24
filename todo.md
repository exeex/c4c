Status: Active
Source Idea Path: ideas/open/prealloc-store-source-publication-planning.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Store-Source Planning Surfaces

# Current Packet

## Just Finished

Completed Step 1 audit for store-source publication planning. Used
`c4c-clang-tools` first to map `dispatch_store_sources.cpp`,
`publication_plans.hpp`, `publication_plans.cpp`, and `module.hpp`, then
checked targeted definitions for the active helper clusters.

Mapped AArch64 store-source helpers:
- Local store-source publication cluster:
  `lower_store_local_value_publication`,
  `store_local_value_is_byval_frame_slot_load`,
  `store_local_value_is_wide_load_from_narrow_local_store`,
  `find_latest_narrow_store_for_wide_local_load`,
  `store_local_value_cast_producer`,
  `store_local_value_has_select_producer`,
  `store_local_value_has_scalar_fp_binary_producer`,
  `prepared_or_emitted_store_value_register`, and
  `store_local_uses_pointer_value_address`.
- Logical local-slot/recovered-source cluster:
  `local_slot_reference_name`, `local_slot_reference_matches`,
  `prepared_frame_slot_object_name`,
  `prepared_load_local_frame_object_name`,
  `value_name_has_slot_prefix`, `parse_trailing_dot_offset`, and
  `store_local_targets_logical_slot`.
- Pointer writeback/publication cluster:
  `lower_stack_homed_pointer_store_writeback`,
  `lower_pointer_base_plus_offset_store_local_publication`,
  `store_local_frame_slot_offset`,
  `prepared_global_symbol_from_value_name`,
  `emit_global_symbol_address_to_register`, and
  `emit_pointer_base_plus_offset_to_register`.
- Global pending-publication cluster:
  `is_store_global_select_snapshot_run_instruction`,
  `lower_pending_store_global_stack_value_publications`, and
  `lower_store_global_value_publication`.

Target-neutral store-source facts found:
- Logical source value and prepared `ValueNameId`/`PreparedValueId` identity for
  the value being stored.
- Store destination memory-access identity from `PreparedMemoryAccess`, including
  `stored_value_name`, `PreparedAddressBaseKind`, frame-slot id, pointer-value
  name, byte offset, size/alignment, and `can_use_base_plus_offset`.
- Destination stack-object/frame-slot identity via prepared stack layout
  (`PreparedFrameSlot`, `PreparedStackObject`) for local stores and recovered
  local-load sources.
- Source home classification via `PreparedValueHome`:
  register, stack slot, rematerializable immediate, and pointer-base-plus-offset
  with base value and byte delta.
- Pending-publication bookkeeping facts: whether a named stack value has already
  been published for store-global recovery, and whether stack-home-only
  publication is requested.
- Same-block source provenance facts that are policy-free when represented as
  facts only: cast/select/scalar-fp-binary producers, direct-global-load presence,
  byval pointer-value load source, and wide-load-from-narrow-store recovery.

AArch64 address/emission facts to keep local:
- Memory operand validation/spelling (`MemoryOperand`, `memory_address`,
  `frame_slot_address`, `register_indirect_address`) and base-plus-offset
  legality as consumed by AArch64 lowering.
- Store mnemonic, scalar view, register spelling/parsing, scratch selection,
  reserved-scratch avoidance, and register-alias/clobber checks.
- Actual publication emission (`emit_value_publication_to_register`,
  `emit_fp_value_to_register`, cast emission, global-symbol address materializing
  instructions, pointer-base-plus-offset materializing instructions).
- Final `InstructionRecord`, `AssemblerInstructionRecord`,
  `MachineInstruction`, side-effect lists, and origin construction.
- Scan policy for pending store-global snapshots:
  `is_store_global_select_snapshot_run_instruction`, calls to
  `lower_memory_instruction`, and insertion into the `MachineBlock`.

Mixed hook-boundary facts:
- Recovered-source lookup for wide-load-from-narrow-local-store mixes neutral
  same-block source/destination identity with lane-offset name parsing; Step 2
  can expose the recovered source as optional plan data, but the first adapter
  should keep name parsing or lane matching behavior unchanged.
- Pointer-store writeback mixes a neutral pointer-value base, stack-home offset,
  byte offset, and source value with AArch64 scratch/address-register emission;
  represent the writeback intent in the plan, lower it locally.
- Pointer-base-plus-offset store-local publication mixes neutral source-home
  kind/base/delta/destination slot facts with target-specific global relocation,
  load/add/sub sequences, and stack-store emission.
- Store-global pending publication mixes neutral duplicate-publication state and
  stack-home-only intent with target scan/lowering policy; record the facts, but
  do not move the scan loop or memory lowering into prealloc.

## Suggested Next

Execute Step 2 by adding a narrow `prepare::PreparedStoreSourcePublicationPlan`
record/helper in `src/backend/prealloc/publication_plans.*`.

Smallest safe first extraction candidate:
- Inputs: source `bir::Value`, optional `PreparedMemoryAccess` for the store
  instruction, optional `PreparedValueHome` for the source value, optional
  recovered source value/index, optional pointer-base home, and a small
  duplicate-publication/stack-home-only input bit for store-global paths.
- Record fields: status, source value/name/id, destination access/address facts,
  destination frame slot/object identity when available, source home kind/storage
  encoding, optional recovered source value/instruction index, pending
  publication/writeback intent, pointer base value name, pointer byte offset, and
  prepared stack offset/slot facts.
- Tests: focused backend MIR test that builds the record from synthetic
  Prepared/BIR facts without including AArch64 codegen headers or machine
  instruction records.

## Watchouts

- Keep AArch64 memory operand spelling, store opcode selection, scratch choice,
  global address materialization, stack-pointer sequences, and final instruction
  construction out of prealloc.
- Do not weaken store expectations or create testcase-shaped shortcuts.
- Do not move same-block scan/emission behavior into the Step 2 helper. The
  helper should describe facts supplied by the caller; AArch64 can remain
  responsible for finding recovered producers until a tighter shared query
  boundary exists.
- Keep `PreparedScalarPublicationPlan` stable unless Step 2 can share a tiny
  storage-encoding helper without changing its behavior.

## Proof

No validation run; this was an audit-only `todo.md` update. No `test_after.log`
was created or touched.
