Status: Active
Source Idea Path: ideas/open/187_bir_memory_provenance_global_handle_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Add focused global provenance tests

# Current Packet

## Just Finished

Step 3 added focused backend LIR-to-BIR tests for the addressed global
provenance LinkNameId boundary introduced by Step 2.

The new test exercises structured lowering through an addressed global pointer
slot, verifies the recovered global load keeps the stored target LinkNameId, and
guards `AddressedGlobalPointerSlots` plus `AddressedGlobalPointerValueSlots`
against stale same-spelling LinkNameId aliases while retaining a distinct
`kInvalidLinkName` raw/no-id compatibility key.

## Suggested Next

Proceed to the next supervisor-selected packet for the active plan. A coherent
next slice would audit nearby dynamic global provenance side tables for the
same LinkNameId-vs-spelling boundary, without widening route-local maps.

## Watchouts

- `GlobalPointerSlotKey::global_name` remains part of the key for raw/no-id
  compatibility and display-boundary bridging; `link_name_id` is authoritative
  when present.
- The test intentionally checks lowering-visible BIR instructions and the
  side-table key maps; it does not rely on final printed names.
- No implementation files or expectation rewrites were changed.

## Proof

Ran exactly:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' | tee test_after.log`

Result: passed. Build completed and backend CTest reported 100% passing for 109
run tests; 12 disabled MIR trace tests were not run. Proof log:
`test_after.log`.
