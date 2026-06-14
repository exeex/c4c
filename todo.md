Status: Active
Source Idea Path: ideas/open/261_phase_f3_x86_route3_loadlocal_publication_fixture_support.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Focused Agreement Proof

# Current Packet

## Just Finished

Completed plan Step 3, `Focused Agreement Proof`.

The existing joined-branch compare-join `EdgeStoreSlot` selected-`LoadLocal`
rows cover the supported production proof surface without additional test
edits:

- positive x86 rendering through
  `render_agreed_route3_load_local_statement_memory_operand(...)` via
  `check_materialized_compare_join_edge_store_slot_selected_loadlocal_x86_route()`
- missing predecessor source address rejection before a selected
  `LoadLocal` compare-join packet is accepted via
  `check_materialized_compare_join_edge_store_slot_selected_loadlocal_rejects_missing_source_memory()`
- join-carrier-only `LoadLocal` drift rejection on the prepared contract side via
  `check_materialized_compare_join_edge_store_slot_selected_loadlocal_rejects_carrier_only_loadlocal()`
- incomplete prepared source-memory publication rejection by the x86
  prepared-module consumer via
  `check_materialized_compare_join_edge_store_slot_selected_loadlocal_x86_rejects_incomplete_source_memory()`

No new rows were kept. Prepared-only, stale-publication, byte-offset drift, and
cross-publication mismatch rows are not naturally expressible through this
production fixture without hand-built or stale prepared publication state, so
they remain recorded as intentionally out of scope for idea 261.

## Suggested Next

Execute plan Step 4, `Hand Back to Idea 258`.

Record that idea 261 now provides the supported joined-branch proof surface
needed for idea 258 Step 4, then route lifecycle handling back through the plan
owner.

## Watchouts

- Keep idea 261 scoped to the supported joined-branch fixture surface; do not
  reopen idea 262's compare-join lowering.
- Do not hand-build `PreparedEdgePublication` rows or stale prepared render
  contract state just to express prepared-only, byte-offset drift, or
  cross-publication mismatch cases.
- Legacy no-publication fallback remains compatibility behavior and was not
  counted as positive Route 3/prepared agreement proof.

## Proof

Delegated proof command passed and wrote `test_after.log`:

```bash
(cmake --build --preset default && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_prepared_printer)$' --output-on-failure && cmake --build build-x86 --target backend_x86_route_debug_test backend_x86_handoff_boundary_test -j2 && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_x86_route_debug|backend_x86_handoff_boundary)$') > test_after.log 2>&1
```

Result: `test_after.log` shows 2/2 default prepared tests and 2/2 x86 tests
passed.
