Status: Active
Source Idea Path: ideas/open/417_prepared_storage_layout_and_initializer_contracts.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Migrate One Local-Memory Consumer Path

# Current Packet

## Just Finished

Step 4 migrated one RV64 local-memory consumer path in
`prepared_call_emit.cpp`: frame-slot-address call arguments with prepared
`FrameSlotAddress` source selection now construct selected local-storage
contract facts from the prepared source selection, indexed address
materialization, frame slot, and frame plan before lowering the address.

The migrated path verifies those facts with
`verify_prepared_selected_local_storage_contract`, emits only after a coherent
report, and fails closed for missing extent/alignment or conflicting byte-range
facts without deriving storage layout from raw BIR/source type shape.

Focused coverage in `backend_riscv_object_emission` directly exercises
`emit_riscv_simple_call` on the migrated prepared-call path, proving coherent
address lowering plus fail-closed missing/incoherent selected-storage facts.

## Suggested Next

Execute Step 5 by migrating the next selected local-memory consumer or producer
boundary chosen by the supervisor, preserving the same prepared-fact-only
contract boundary and avoiding target-side recovery of raw layout facts.

## Watchouts

- This packet intentionally migrated only the `FrameSlotAddress` prepared-call
  consumer in `prepared_call_emit.cpp`; the existing
  `LocalFrameAddressMaterialization`/byval route remains on its prior lowering
  behavior so the byval route tests stay unchanged.
- `object_emission.cpp` has a separate frame-slot-address object path and was
  not touched because it was outside this executor packet's owned files.
- The selected object-data verifier facts remain API-only after this packet;
  object-data producers/consumers still need their own migration packet.

## Proof

Ran exactly:
`( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prealloc_prepared_contract_verifier$|backend_riscv_object_emission$|backend_(dump|codegen_route)_riscv64_byval_|backend_(dump|codegen_route)_riscv64_aggregate_local_)' ) > test_after.log 2>&1`

Result: passed. `test_after.log` records 14/14 selected tests passing,
including `backend_prealloc_prepared_contract_verifier`,
`backend_riscv_object_emission`, the RV64 byval dump/codegen-route tests, and
the RV64 aggregate-local dump/codegen-route tests.
