Status: Active
Source Idea Path: ideas/open/650_edge_store_local_aggregate_publication_ordering.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Split The Narrow Publication-Ordering Owner

# Current Packet

## Just Finished

Completed Step 3 implementation for narrow RV64 consumer admission of
authoritative `edge_store_slot` carriers.

Implementation:

- Added RV64 object-route admission helpers in
  `src/backend/mir/riscv/codegen/object_emission.cpp` that identify synthetic
  carrier `StoreLocalInst` / `LoadLocalInst` operations by exact slot/storage
  match against an authoritative `PreparedJoinTransfer` with
  `carrier_kind=EdgeStoreSlot`, valid true/false transfer indexes, published
  predecessor edge-copy bundles, and a GPR-homed destination value.
- The carrier `StoreLocalInst` and `LoadLocalInst` now emit empty fragments
  only under that authority. Ordinary local memory still flows through
  `prepared_memory_access_for_local_instruction(...)`; no fake
  `PreparedMemoryAccess` is synthesized for `%*.phi` carrier slots.
- Extended the existing out-of-SSA move emitter to accept
  `phi_loop_carry_register_to_register` and
  `phi_loop_carry_immediate_materialization` with the same constraints already
  used for `phi_join_*` edge-copy moves.
- Added focused coverage in
  `tests/backend/case/riscv64_edge_store_local_publication_ordering.c` and
  registered `backend_dump_riscv64_edge_store_local_publication_ordering` plus
  `backend_cli_riscv64_edge_store_local_publication_ordering`.

Focused positive coverage proves the route by requiring the prepared dump to
show the `loop_carry` `edge_store_slot` join transfer, both predecessor
edge-copy moves, a GPR home for `%t24`, and `missing_destination_access` for the
carrier stores while forbidding prepared `access` records for the carrier
store/load positions. The object test proves RV64 object emission succeeds
through that virtual carrier route.

Representative status: both `pr68185.c` and `pr68321.c` now emit RV64 objects
past the previous `unsupported_local_memory_access` owner into
`build/agent_state/650_step3_edge_store/`.

## Suggested Next

Proceed to Step 4 representative integration proof. Capture focused test
results plus representative disassembly/evidence for `pr68185.c` and
`pr68321.c`, then record whether any downstream owner remains after object
emission advances.

## Watchouts

- The Step 3 fix deliberately admits only virtual `edge_store_slot` carriers;
  it does not authorize arbitrary local-memory accesses without prepared
  frame-slot or pointer-value addressing.
- Keep the exact storage-name match, published edge-copy requirement,
  true/false transfer-index requirement, and GPR destination-home requirement
  intact for fail-closed behavior.
- `loop_carry` edge-copy move reasons are now accepted only inside the existing
  out-of-SSA parallel-copy emitter and still require the same move shape,
  destination storage, step-index, immediate/register, and cycle-temp checks as
  the existing `phi_join_*` moves.
- The focused dump intentionally still shows `missing_destination_access` for
  the carrier stores; that is evidence that the route did not create a fake
  frame-slot access.

## Proof

Passed:

```sh
bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_(dump|cli)_riscv64_edge_store_local_publication_ordering$" && mkdir -p build/agent_state/650_step3_edge_store && build/c4cll --target riscv64-linux-gnu --codegen obj tests/c/external/gcc_torture/src/pr68185.c -o build/agent_state/650_step3_edge_store/pr68185.o && build/c4cll --target riscv64-linux-gnu --codegen obj tests/c/external/gcc_torture/src/pr68321.c -o build/agent_state/650_step3_edge_store/pr68321.o; } 2>&1 | tee test_after.log'
```

`test_after.log` contains the canonical proof log. The focused CTest subset
passed 2/2, and both representative object-emission commands exited 0.
