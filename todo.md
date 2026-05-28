Status: Active
Source Idea Path: ideas/open/67_aarch64_local_slot_address_offset_probe.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Apply Only The Classification-Sized Follow-Up

# Current Packet

## Just Finished

Step 4 completed: applied the classification-sized follow-up by deleting only
the unreachable `local_slot_address_frame_offset` API surface.

Changes made:
- Removed the `local_slot_address_frame_offset` declaration from
  `dispatch_publication.hpp`.
- Removed the null `local_slot_address_frame_offset` definition from
  `dispatch_publication.cpp`.
- Left `local_aggregate_address_frame_offset`, prepared frame-address
  materialization, local aggregate address publication, call-source-selection
  behavior, and tests unchanged.

Post-delete evidence:
- `rg` finds no remaining `local_slot_address_frame_offset(` references under
  `src` or `tests`.
- The Step 2 runtime probe remains in place to prove current local-slot pointer
  Add/Sub callers use prepared frame-address materialization and fail closed
  without those facts.

## Suggested Next

Step 5 - Prove Local-Slot Offset Classification: run the delegated closure
proof, confirm `local_slot_address_frame_offset` remains absent, confirm the
Step 2 probe still covers local-slot pointer Add/Sub prepared frame-address
materialization and incomplete-facts fail-closed behavior, and scan the diff
for unrelated frame/address authority or test expectation changes.

## Watchouts

Do not broaden closure into shared frame/address authority or adjacent
aggregate helper cleanup. The only code deletion in this idea so far is the
dead `local_slot_address_frame_offset` API surface. Keep the Step 2 runtime
probe as classification evidence and avoid treating it as new capability
implementation.

## Proof

Proof passed:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_records|backend_aarch64_prepared_memory_operand_records|backend_aarch64_memory_operand_contract|backend_codegen_route_aarch64_local_aggregate_address_pointer_copy_publishes_frame_address|backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor)$') > test_after.log 2>&1`

`test_after.log` reports 6/6 tests passed. `git diff --check` passed.

Route scan:
`rg -n "local_slot_address_frame_offset\\(" src tests -g '!review/**'`
found no remaining references.
