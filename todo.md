Status: Active
Source Idea Path: ideas/open/89_aarch64_memory_store_retargeting_owner.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract Pointer Store Retargeting

# Current Packet

## Just Finished

Step 2 extracted only the pointer/emitted-scalar store-retargeting helpers into
`src/backend/mir/aarch64/codegen/memory_store_retargeting.hpp` and
`src/backend/mir/aarch64/codegen/memory_store_retargeting.cpp`.

Moved helpers:

- Pointer store-value/address retargeting:
  `store_local_uses_pointer_value_address`,
  `retarget_pointer_store_value_to_materialized_address`,
  `retarget_store_address_to_materialized_pointer`.
- Emitted-scalar store reuse:
  `prepared_or_emitted_store_value_register`,
  `retarget_pointer_store_value_to_emitted_scalar`,
  `retarget_store_local_value_to_emitted_scalar`.

Wiring completed:

- `lower_store_local_value_publication` calls
  `store_local_uses_pointer_value_address` and
  `prepared_or_emitted_store_value_register` to avoid redundant source
  publication.
- `retarget_store_local_value_to_emitted_scalar` calls
  `store_local_uses_pointer_value_address` and
  `prepared_or_emitted_store_value_register`.
- `dispatch.cpp` still calls the four dispatch-side retargeting helpers
  explicitly through the existing `memory.hpp` include path.
- `memory.cpp` includes the new owner header directly.
- `memory.hpp` no longer owns the six declarations; it includes the new owner
  header to preserve existing external consumers.
- `src/backend/CMakeLists.txt` builds `memory_store_retargeting.cpp`.
- Stack-layout and local-address helpers stayed in `memory.cpp`.
- Lookup/order behavior is unchanged: emitted scalar is preferred before
  prepared home, and non-matching store/record shapes still no-op.

## Suggested Next

Suggested next packet: Step 3 should decide and implement the stack-layout and
local-address helper boundary, including whether `resolve_frame_slot_memory_offset`
moves with `apply_stack_layout_to_memory_record` or remains in `memory.cpp`.

## Watchouts

- `apply_stack_layout_to_memory_record` currently depends on the private
  `resolve_frame_slot_memory_offset` helper in `memory.cpp`; moving stack-layout
  application without that dependency plan will either widen the owner or create
  an awkward callback surface.
- Keep the route memory-local; do not fold store publication, edge-copy,
  prepared-wrapper work, or generic dispatch authority into the retargeting
  owner.

## Proof

Ran the supervisor-selected Step 2 proof successfully. `test_after.log`
contains the CTest output and is the preserved proof log.

```bash
cmake --build build --target backend_aarch64_prepared_memory_operand_records_test backend_aarch64_instruction_dispatch_test && ctest --test-dir build -R '^(backend_aarch64_prepared_memory_operand_records|backend_aarch64_instruction_dispatch)$' --output-on-failure > test_after.log
```
