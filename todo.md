Status: Active
Source Idea Path: ideas/open/89_aarch64_memory_store_retargeting_owner.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Tighten Surface and Validate Acceptance

# Current Packet

## Just Finished

Step 4 moved the existing prepared lookup definitions
`find_frame_slot_by_id` and `find_stack_object_by_id` out of the
`prepared_lookups.cpp` anonymous namespace and added matching declarations to
`prepared_lookups.hpp`. `memory_store_retargeting.cpp` now calls the prepared
lookup authority directly and no longer includes `memory.hpp`; `memory.hpp` no
longer re-exports `memory_store_retargeting.hpp`; `dispatch.cpp` includes
`memory_store_retargeting.hpp` directly.

Step 4 also resolved the local-helper name collisions exposed by the public
prepared lookup declarations: `cast_ops.cpp` renamed its local frame-slot
helper, and `call_plans.cpp` renamed its local frame-slot helper. No lookup
behavior changed.

## Suggested Next

Suggested next packet: supervisor review/commit for the completed Step 4
surface-tightening slice, then lifecycle handling for the active plan state.

## Watchouts

- `memory_store_retargeting.cpp` now depends on the prepared lookup owner for
  frame-slot/object lookup authority instead of reaching through `memory.hpp`.
- `memory.hpp` no longer provides compatibility inclusion for
  `memory_store_retargeting.hpp`; direct consumers should include the
  retargeting header explicitly.

## Proof

Ran the supervisor-selected Step 4 proof command successfully. Build passed and
CTest passed both focused tests. Proof log: `test_after.log`.

```bash
cmake --build build --target backend_aarch64_prepared_memory_operand_records_test backend_aarch64_instruction_dispatch_test && ctest --test-dir build -R '^(backend_aarch64_prepared_memory_operand_records|backend_aarch64_instruction_dispatch)$' --output-on-failure > test_after.log
```
