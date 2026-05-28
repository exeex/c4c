Status: Active
Source Idea Path: ideas/open/67_aarch64_local_slot_address_offset_probe.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Build A Narrow Reachability Probe

# Current Packet

## Just Finished

Step 2 completed: added a narrow runtime reachability probe for the local-slot
address offset path without deleting or implementing the null helper.

Probe added:
- `local_slot_address_offset_probe_uses_prepared_frame_materialization` in
  `backend_aarch64_instruction_dispatch_test.cpp`.
- The positive case builds a local-slot pointer `Add` where `%slot.base` has a
  prepared `FrameSlot` address materialization at instruction 0 and
  `%slot.addr = %slot.base + 16` is lowered at instruction 1.
- The test attaches prepared function lookups and dispatches the block through
  the normal AArch64 route. It expects both the prepared base address and the
  adjusted local-slot address to print: `add x13, sp, #64` and
  `add x14, sp, #80`.
- The negative case removes the prepared frame-address materialization facts
  and verifies the route does not rederive `add x14, sp, #80` from local-slot
  identity alone.

Evidence gathered:
- Current local-slot pointer Add callers can lower through prepared
  frame-address materialization lookups and `lower_local_slot_address_publication`
  without needing `local_slot_address_frame_offset`.
- Incomplete prepared frame-address facts fail closed for the adjusted
  local-slot address rather than falling back to local-slot-name offset
  derivation.
- A post-probe scan still finds `local_slot_address_frame_offset` only at its
  declaration and null definition, plus the new probe names in the test file.
  This is reachability evidence only; final classification remains Step 3.

## Suggested Next

Step 3 - Classify The Path Outcome: classify `local_slot_address_frame_offset`
using the Step 1 static caller inventory plus the Step 2 runtime evidence. The
classification should decide whether the next route is deleting unreachable
API surface, keeping the disabled/null helper documented, or implementing a
missing prepared offset path. Do not make the classification by changing source
behavior in Step 3 unless the supervisor delegates a narrow cleanup.

## Watchouts

Do not treat the Step 2 probe as capability implementation: it proves the
current prepared-query route for local-slot pointer Add/Sub and the incomplete
facts fail-closed behavior. Step 3 still needs to explicitly classify the null
helper and choose the follow-up. Do not mutate shared frame-address authority,
weaken tests, or bundle unrelated dispatch-family contraction.

## Proof

Proof passed:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_records|backend_aarch64_prepared_memory_operand_records|backend_aarch64_memory_operand_contract|backend_codegen_route_aarch64_local_aggregate_address_pointer_copy_publishes_frame_address|backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor)$') > test_after.log 2>&1`

`test_after.log` reports 6/6 tests passed. `git diff --check` passed.

Additional local check:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_aarch64_instruction_dispatch$'`
passed before the full delegated proof.

Route scan:
`rg -n "local_slot_address_frame_offset\\(|local_slot_address_offset_probe_uses_prepared_frame_materialization|prepared_with_local_slot_address_offset_probe" tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp src/backend/mir/aarch64/codegen/dispatch_publication.cpp src/backend/mir/aarch64/codegen/dispatch_publication.hpp`
confirmed the helper remains declaration/definition only and the new hits are
the probe helper/test names.
