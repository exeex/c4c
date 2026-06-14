Status: Active
Source Idea Path: ideas/open/261_phase_f3_x86_route3_loadlocal_publication_fixture_support.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Fixture or Helper Implementation

# Current Packet

## Just Finished

Completed plan Step 2, `Fixture or Helper Implementation`.

The minimal supported fixture surface already exists through the resumed
joined-branch compare-join `EdgeStoreSlot` helpers added with idea 262:
`make_x86_param_eq_zero_branch_joined_loadlocal_or_sub_then_add_module()` and
`check_materialized_compare_join_edge_store_slot_selected_loadlocal_*()` now
exercise a selected same-predecessor-block `LoadLocal` with prepared local-slot
addressing through the production x86 Route 3 agreement facade. No extra helper
or production edit was needed for idea 261 Step 2.

## Suggested Next

Execute plan Step 3, `Focused Agreement Proof`.

Use the existing selected-`LoadLocal` joined-branch fixture rows to prove the
positive Route 3/prepared agreement path and any reachable fail-closed rows
needed by idea 258.

## Watchouts

- Do not revive the rejected synthetic `join_transfers` route from the
  addressed local-slot guard fixture.
- Do not count legacy no-publication fallback as positive Route 3/prepared
  agreement.
- Do not reopen idea 262's generic selected-arm scope; use the bridge it
  already landed.
- Reachable rows already present in the joined-branch fixture include positive
  x86 rendering, missing source address, join-carrier-only drift, and
  incomplete prepared source-memory publication. Prepared-only or stale
  publication mismatches must not be hand-built if the supported route cannot
  express them naturally.

## Proof

Delegated proof command passed and wrote `test_after.log`:

```bash
(cmake --build --preset default && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_prepared_printer)$' --output-on-failure && cmake --build build-x86 --target backend_x86_route_debug_test backend_x86_handoff_boundary_test -j2 && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_x86_route_debug|backend_x86_handoff_boundary)$') > test_after.log 2>&1
```

Result: `test_after.log` shows 2/2 default prepared tests and 2/2 x86 tests
passed.
