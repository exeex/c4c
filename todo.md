# Current Packet

Status: Active
Source Idea Path: ideas/open/26_riscv_prepared_edge_publication_pointer_base_register_consumer.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Fail-Closed Authority

## Just Finished

Completed Step 2 and Step 3 for the RISC-V pointer-base prepared
edge-publication route. The implemented policy accepts
`PointerBasePlusOffset -> Register` only when the source home has
`pointer_base_value_name`, has `pointer_byte_delta`, resolves the base name
through `PreparedValueHomeLookups::value_ids` and `homes_by_id` to a register
home, and the delta fits RISC-V signed-12-bit `addi`.

The RISC-V consumer now records pointer-base provenance in
`EdgePublicationMoveIntent` and emits `addi <dst>, <base>, <delta>` for
non-zero deltas or `mv <dst>, <base>` for zero delta after shared
`edge_publications` authority accepts the move. Existing `Register -> Register`,
`RematerializableImmediate -> Register`, and focused `StackSlot -> Register`
behavior is preserved.

Focused coverage now proves the positive shared lookup-backed pointer-base path
and fail-closed behavior for missing base name, unresolved base name,
non-register base home, missing delta, out-of-range signed-12-bit delta, missing
publication authority/local rediscovery, non-move publications, and stack-slot
destinations.

## Suggested Next

Proceed to Step 5 validation or reviewer handoff for the pointer-base slice,
using `test_after.log` as the focused proof artifact.

## Watchouts

Do not implement source-to-`StackSlot` destinations or stack-source policy
broadening in this route. Do not rediscover edge facts locally; the shared
`edge_publications` lookup remains the semantic authority.

The pointer-base policy is intentionally narrow: register base only, register
destination only, signed-12-bit immediate delta only. Source-to-`StackSlot`
destinations, stack-source policy broadening, non-register bases, and deltas
requiring multi-instruction materialization remain unsupported/fail-closed.

## Proof

Ran the supervisor-selected focused proof:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_prepared_edge_publication|backend_codegen_route_riscv64_.*|backend_prepared_lookup_helper|backend_prealloc_block_entry_publications)$' >> test_after.log 2>&1`.

Result: PASS, 5/5 selected tests passed. Proof log: `test_after.log`.

Supervisor ran the matching regression guard against the focused
`test_before.log` and focused `test_after.log` with non-decreasing pass count
allowed because this packet extended an existing CTest binary. Result: PASS
with 5/5 before and 5/5 after, no new failures.

Supervisor then ran broader backend validation:

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`

Result: PASS. Build succeeded and CTest reported 163/163 backend tests passing.
This backend run is broader validation only, not the matching before/after
regression comparison for the focused packet.
