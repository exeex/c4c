Status: Active
Source Idea Path: ideas/open/212_route5_edge_join_oracle_printer_row.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Add Route-Native Evidence for the Row

# Current Packet

## Just Finished

Step 2 added Route 5 evidence support for only the selected
`PreparedCurrentBlockJoinParallelCopySourceFact` named current-block join row
shape. `PreparedCurrentBlockJoinParallelCopySourceQueryInputs` now accepts an
optional `bir::Route5EdgeJoinSourceIndex`, and the selected fact records an
index-owned `Route5CurrentBlockJoinSourceRecord` only when the Route 5
successor PHI destination/source identity agrees with the prepared predecessor,
successor, edge publication destination/source values, source/destination value
ids and names, and source/destination homes.

The helper remains fail-closed: absent Route 5 input leaves the prepared row
unchanged, duplicate agreeing Route 5 records do not attach evidence, wrong
predecessor evidence falls back to the prepared fact, and adjacent immediate,
stack/source-value, and unsupported facts remain prepared-owned.

## Suggested Next

Execute Step 3 from `plan.md`: add focused fail-closed proof for the selected
row. The next packet should cover route/prepared disagreement dimensions that
are not yet explicit in `backend_prepared_lookup_helper`, especially no-source
or absent Route 5 input retaining the prepared row, source/destination mismatch,
and any memory-source-only or unsupported-source shape that is representable for
this row without moving branch, parallel-copy, move-bundle, wrapper, or output
ownership into Route 5.

Recommended proof command:

```bash
cmake --build --preset default --target backend_prepared_lookup_helper_test backend_prepared_printer_test backend_prealloc_block_entry_publications_test backend_aarch64_current_block_join_routing_test backend_prepare_authoritative_join_ownership_test backend_x86_prepared_handoff_label_authority_test backend_riscv_prepared_edge_publication_test && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_prepared_printer|backend_prealloc_block_entry_publications|backend_aarch64_current_block_join_routing|backend_prepare_authoritative_join_ownership|backend_x86_prepared_handoff_label_authority|backend_riscv_prepared_edge_publication)$' > test_after.log
```

## Watchouts

- Keep Step 3 to the one selected helper-oracle row class. Do not migrate the
  whole edge-publication, source-producer, move-bundle, joined-branch, wrapper,
  or printer families.
- The implementation intentionally exposes Route 5 as helper-oracle metadata
  only; prepared edge publication, move-bundle, branch, parallel-copy, printer,
  and wrapper ownership remain unchanged.
- Current Step 2 tests prove positive agreement, prepared-only fallback,
  duplicate/conflict fallback, wrong-predecessor fallback, and adjacent
  immediate/stack/unsupported prepared ownership. They do not yet separately
  cover every mismatch dimension listed by Step 3.
- `clang-format` is not installed in this environment, so no formatter was run.

## Proof

Step 2 proof passed with the exact delegated command:

```bash
cmake --build --preset default --target backend_prepared_lookup_helper_test backend_prepared_printer_test backend_prealloc_block_entry_publications_test backend_aarch64_current_block_join_routing_test backend_prepare_authoritative_join_ownership_test backend_x86_prepared_handoff_label_authority_test backend_riscv_prepared_edge_publication_test && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_prepared_printer|backend_prealloc_block_entry_publications|backend_aarch64_current_block_join_routing|backend_prepare_authoritative_join_ownership|backend_x86_prepared_handoff_label_authority|backend_riscv_prepared_edge_publication)$' > test_after.log
```

`test_after.log` records 7/7 passing:
`backend_aarch64_current_block_join_routing`,
`backend_riscv_prepared_edge_publication`, `backend_prepared_printer`,
`backend_prealloc_block_entry_publications`, `backend_prepared_lookup_helper`,
`backend_x86_prepared_handoff_label_authority`, and
`backend_prepare_authoritative_join_ownership`.
