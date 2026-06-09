# Current Packet

Status: Active
Source Idea Path: ideas/open/148_same_block_load_local_stored_value_owner.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map the current API owner and prepared dependencies

## Just Finished

Step 1 inspection mapped the current owner and move dependencies without moving
code.

- Old owner: `PreparedSameBlockLoadLocalStoredValueSource` is declared in
  `src/backend/prealloc/prepared_lookups.hpp` and
  `find_prepared_same_block_load_local_stored_value_source` is declared in the
  same header, then defined in `src/backend/prealloc/prepared_lookups.cpp`.
- Proposed narrow declaration owner: `src/backend/prealloc/addressing.hpp`,
  because the API returns `PreparedMemoryAccess` facts and queries
  `PreparedAddressingFunction` through `find_prepared_memory_access`.
- Selected existing implementation file for Step 2:
  `src/backend/prealloc/prepared_lookups.cpp`. There is no existing
  `src/backend/prealloc/addressing.cpp`; the current compiled implementation
  owner for non-inline helpers declared from `addressing.hpp` is already
  `prepared_lookups.cpp` (`make_prepared_address_materialization_lookups`,
  `make_prepared_memory_access_lookups`, indexed memory-access finders, and
  same-block global load helpers). Creating a true `addressing.cpp` is a
  separate owner-split decision.
- Direct API consumer: `src/backend/mir/aarch64/codegen/calls.cpp` in
  `find_prepared_indirect_callee_stored_value_source`. AST callers found this
  as the direct caller, and AST type refs found the struct used there as the
  return type.
- Current consumer include: `calls.cpp` includes
  `../../../prealloc/prepared_lookups.hpp` for this API. After the declaration
  move, this consumer should include `../../../prealloc/addressing.hpp` only if
  it does not still need unrelated `prepared_lookups.hpp` declarations.
- Prepared source-producer facts reused by the finder:
  `PreparedEdgePublicationSourceProducerLookups` from publication plans,
  `find_prepared_same_block_scalar_producer` from select-chain lookups, and the
  resulting `PreparedEdgePublicationSourceProducer` fields `kind`,
  `load_local`, `block_label`, and `instruction_index`. The move should keep
  this path and must not rescan source producers.
- Prepared stack-layout/addressing facts reused by the finder:
  `PreparedStackLayout`, `find_frame_slot_by_id`/frame-slot offsets,
  `PreparedAddressingFunction`, `find_prepared_memory_access`,
  `PreparedMemoryAccess::address.frame_slot_id`, `byte_offset`, and
  `size_bytes`. The local range helpers currently compute frame-slot-backed
  absolute offsets and compare overlap/exact range match from those prepared
  facts.
- Related existing helper to avoid duplicating blindly:
  `publication_plans.cpp` has same-family load-local/source-producer and
  frame-slot/range helper logic for publication recovery, but it is
  publication-specific and is not the selected owner for this stored-value API.

## Suggested Next

Execute Step 2 by moving the struct and finder declaration from
`prepared_lookups.hpp` into `addressing.hpp`, then update
`prepared_lookups.cpp` includes/definitions just enough for that declaration
move to compile. Keep the existing function signature and prepared-fact reuse
unchanged.

## Watchouts

- `addressing.hpp` currently does not include publication source-producer
  declarations; Step 2 will need either narrow forward declarations or a narrow
  include without pulling unrelated prepared-lookup APIs into addressing.
- Do not duplicate stack-layout or same-block source-producer scans. Reuse
  `find_prepared_same_block_scalar_producer` and prepared memory-access facts.
- Do not special-case the AArch64 calls consumer; it is only the current direct
  consumer, not the reason for the API shape.
- Do not move unrelated same-block materialization APIs from
  `select_chain_lookups.hpp` or publication-specific helpers as part of the
  Step 2 owner move.

## Proof

Inspection-only proof per packet. Ran `rg` plus AST-backed
`c4c-clang-tool[-ccdb]` declaration, definition, caller, callee, signature, and
type-ref queries around
`PreparedSameBlockLoadLocalStoredValueSource` and
`find_prepared_same_block_load_local_stored_value_source`. No build/tests were
required and no `test_after.log` was generated because only `todo.md` changed.
