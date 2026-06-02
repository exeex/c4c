Status: Active
Source Idea Path: ideas/open/89_aarch64_memory_store_retargeting_owner.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Extract Stack-Layout Store Record Rewrites

# Current Packet

## Just Finished

Step 3 extracted the stack-layout/local-address store record rewrite helpers
from `memory.cpp` into
`src/backend/mir/aarch64/codegen/memory_store_retargeting.hpp` and
`src/backend/mir/aarch64/codegen/memory_store_retargeting.cpp`.

Moved helpers:

- Store record stack-layout application:
  `apply_stack_layout_to_memory_record`.
- Local-address store-value rewrite support:
  `find_prepared_local_address_store_value` and
  `rewrite_local_address_store_value`.
- Frame-slot offset resolution:
  `resolve_frame_slot_memory_offset`.

Wiring completed:

- `lower_memory_instruction` still applies stack-layout rewrites through
  `apply_stack_layout_to_memory_record`.
- `lower_f128_transport_instruction` keeps its existing narrow frame-slot
  offset behavior through the owner-exposed `resolve_frame_slot_memory_offset`.
- Frame-slot and prepared-address facts are still consumed from the existing
  prepared inputs; no callback plumbing or duplicated lookup path was added.
- Non-store, non-local-store, and no-prepared-address cases preserve their
  no-op success behavior.

## Suggested Next

Suggested next packet: Step 4 should extract the next bounded memory store
retargeting helper group named by the active plan, keeping dispatch and generic
memory lowering authority out of the store-retargeting owner.

## Watchouts

- `memory_store_retargeting.cpp` now depends on `memory.hpp` for the existing
  frame-slot/object lookup declarations; do not duplicate those lookup helpers
  in the owner.
- Keep the route memory-local; do not fold store publication, edge-copy,
  prepared-wrapper work, or generic dispatch authority into the retargeting
  owner.

## Proof

Ran the supervisor-selected Step 3 proof successfully. `test_after.log`
contains the CTest output and is the preserved proof log.

```bash
cmake --build build --target backend_aarch64_prepared_memory_operand_records_test backend_aarch64_instruction_dispatch_test && ctest --test-dir build -R '^(backend_aarch64_prepared_memory_operand_records|backend_aarch64_instruction_dispatch)$' --output-on-failure > test_after.log
```
