Status: Active
Source Idea Path: ideas/open/04_bir-memory-entrypoints-and-helper-boundaries.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Remove Macro-Included Member Declarations

# Current Packet

## Just Finished

Step 1 of `plan.md` inspected the current BIR memory lowering boundary. The
runbook paths point at the old `src/c4c/lir_to_bir/...` layout; the active
files are `src/backend/bir/lir_to_bir/lowering.hpp`,
`src/backend/bir/lir_to_bir/memory/memory_helpers.hpp`, and memory `.cpp`
files under `src/backend/bir/lir_to_bir/memory/`.

Current include/declaration pattern:
- `lowering.hpp` explicitly declares most memory entrypoints/helpers, then
  defines `C4C_BACKEND_BIR_LIR_TO_BIR_MEMORY_HELPERS_MEMBERS` and includes
  `memory/memory_helpers.hpp` inside `BirFunctionLowerer`.
- `memory_helpers.hpp` has two modes: normal include mode declares pure helper
  structs/functions and includes `../lowering.hpp`; member-fragment mode emits
  `static BirFunctionLowerer` member declarations guarded by
  `C4C_BACKEND_BIR_LIR_TO_BIR_MEMORY_HELPERS_MEMBERS_INCLUDED`.
- `addressing.cpp`, `local_slots.cpp`, and `provenance.cpp` include
  `memory_helpers.hpp` normally for pure helper declarations.

Member declarations that must become explicit in `lowering.hpp` when the macro
include is removed:
- `find_repeated_aggregate_extent_at_offset(...)`
- `find_nested_repeated_aggregate_extent_at_offset(...)`
- `resolve_relative_gep_target(...)`
- `find_pointer_array_length_at_offset(...)`
- `resolve_global_gep_address(...)`
- `resolve_relative_global_gep_address(...)`
- `resolve_global_dynamic_pointer_array_access(...)`
- `resolve_global_dynamic_aggregate_array_access(...)`

Memory member entrypoints already explicitly declared in `lowering.hpp` and
should stay stateful lowerer members:
- `lower_local_memory_alloca_inst(...)`
- `lower_memory_gep_inst(...)`
- `lower_memory_store_inst(...)`
- `lower_memory_load_inst(...)`
- `lower_memory_memcpy_inst(...)`
- `lower_memory_memset_inst(...)`
- `try_lower_direct_memory_intrinsic_call(...)`
- coordinator entrypoint `lower_scalar_or_local_memory_inst(...)`

Pure helper declarations that should remain in normal `memory_helpers.hpp`:
- `ScalarLayoutLeafFacts`
- `ScalarLayoutByteOffsetFacts`
- `AggregateByteOffsetProjection`
- `resolve_scalar_layout_facts_at_byte_offset(...)`
- `resolve_aggregate_byte_offset_projection(...)`
- `can_reinterpret_byte_storage_as_type(...)`

`local_slots.cpp` movement candidates: none are safe to move mechanically in
the next packet. It still owns mixed alloca, local-slot load/store, memcpy,
memset, local aggregate GEP, dynamic aggregate, and local pointer-array
families with shared anonymous helper structs/functions; movement should wait
until the declaration-boundary cleanup compiles.

## Suggested Next

Delegate Step 2 to an executor: replace the member-fragment include in
`lowering.hpp` with explicit declarations for the eight macro-included member
helpers listed above, then remove the member-fragment mode from
`memory_helpers.hpp`.

## Watchouts

- Keep the next packet scoped to declarations only; behavior-preserving compile
  proof should catch signature drift.
- Do not convert the eight macro-included declarations into broad free helpers
  taking `BirFunctionLowerer& self`; they are currently `BirFunctionLowerer`
  static member helpers and can be made explicit as-is first.
- Normal `memory_helpers.hpp` declarations depend on `BirFunctionLowerer` nested
  types, so include-order/cycle handling matters when simplifying the header.
- The active repo path is `src/backend/bir/lir_to_bir/...`, not the stale
  `src/c4c/lir_to_bir/...` path in the runbook.

## Proof

Read-only inspection packet. No build/test proof required by supervisor, and no
`test_after.log` was created.
