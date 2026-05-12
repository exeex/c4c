Status: Active
Source Idea Path: ideas/open/187_bir_memory_provenance_global_handle_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Fence selected global provenance identity

# Current Packet

## Just Finished

Step 2 hardened the selected addressed global provenance side tables:
`AddressedGlobalPointerSlots` and `AddressedGlobalPointerValueSlots` now key
through `GlobalPointerSlotKey{link_name_id, global_name, byte_offset}` via
`make_global_pointer_slot_key(const GlobalAddress&)`.

Metadata-rich generated global provenance now carries `LinkNameId` into the
side-table identity, so stale or mismatched ids do not collide with another
entry that only shares raw spelling and byte offset. Raw/no-id compatibility is
preserved as an explicit `kInvalidLinkName` key path, keeping textual global
operands distinguishable from structured generated globals.

## Suggested Next

Proceed to the next supervisor-selected packet for the active plan. A coherent
next slice would audit nearby dynamic global provenance side tables for the
same LinkNameId-vs-spelling boundary, without widening route-local maps.

## Watchouts

- `GlobalPointerSlotKey::global_name` remains part of the key for raw/no-id
  compatibility and display-boundary bridging; `link_name_id` is authoritative
  when present.
- This slice intentionally did not alter direct BIR global op emission, which
  already emits `global_name_id` through `link_name_id_for_global`.
- No test expectations were changed.

## Proof

Ran exactly:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' | tee test_after.log`

Result: passed. Build completed and backend CTest reported 100% passing for 109
run tests; 12 disabled MIR trace tests were not run. Proof log:
`test_after.log`.
