Status: Active
Source Idea Path: ideas/open/212_route5_edge_join_oracle_printer_row.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Name the Row and Baseline Prepared Output

# Current Packet

## Just Finished

Step 1 selected one helper-oracle row class for idea 212:
`PreparedCurrentBlockJoinParallelCopySourceFact` emitted by
`prepare_current_block_join_parallel_copy_source_facts(...)` for the named
current-block join source case. The prepared baseline success row is the
available named-source fact for `%current.destination <- %current.incoming`:
`status=available predecessor=current_join.pred successor=current_join.succ
destination_value_id=103 destination=%current.destination
source_value_id=101 source=%current.incoming source_home=register(x10)
destination_home=register(x12) destination_storage=register
destination_register=w12 source_is_incoming_expression=yes
destination_is_source_value=yes source_is_source_value=no move=block_entry
publication=edge_join`.

The row is currently built from prepared edge/join data: prepared join
transfers, prepared block-entry move bundles, shared edge publication lookups,
prepared value homes, and prepared regalloc names. Route 5 evidence for the
same row comes from `bir::route5_current_block_join_source_records(...)`,
`bir::route5_build_edge_join_source_index(...)`, and
`mir::find_bir_current_block_join_source_identity(...)`; it identifies the same
successor PHI destination/source pair and same source producer instruction but
does not own branch policy, parallel-copy scheduling, move-bundle mechanics, or
wrapper output.

## Suggested Next

Execute Step 2 from `plan.md`: let the selected
`PreparedCurrentBlockJoinParallelCopySourceFact` row use Route 5 evidence only
when the Route 5 successor PHI destination/source identity agrees with the
prepared edge publication, prepared move, source/destination value ids or
names, and destination/source home facts. Recommended narrow proof command for
the first implementation packet:

```bash
cmake --build --preset default --target backend_prepared_lookup_helper_test backend_prepared_printer_test backend_prealloc_block_entry_publications_test backend_aarch64_current_block_join_routing_test backend_prepare_authoritative_join_ownership_test backend_x86_prepared_handoff_label_authority_test backend_riscv_prepared_edge_publication_test && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_prepared_printer|backend_prealloc_block_entry_publications|backend_aarch64_current_block_join_routing|backend_prepare_authoritative_join_ownership|backend_x86_prepared_handoff_label_authority|backend_riscv_prepared_edge_publication)$' > test_after.log
```

## Watchouts

- Keep Step 2 to the one selected helper-oracle row class. Do not migrate the
  whole edge-publication, source-producer, move-bundle, joined-branch, wrapper,
  or printer families.
- Prepared fallback matrix for this row:
  - no-source or absent Route 5 join facts: keep the prepared fact/status from
    `prepare_current_block_join_parallel_copy_source_facts(...)`;
  - memory-source Route 5 evidence: treat as non-authoritative for this row
    until it agrees with prepared source-home/source-memory facts;
  - unsupported move: keep the existing
    `PreparedEdgeCopySourceFactsStatus::UnsupportedMove` fact;
  - duplicate or conflicting Route 5 join records: fail closed to prepared,
    matching the current indexed Route 5 duplicate rejection;
  - route/prepared mismatch by predecessor, destination type/name, source
    name/type, value id/name, move, or home: keep the prepared row;
  - missing edge publication lookups, value locations, names, block, or
    successor label: keep the existing prepared query status.
- Existing coverage: `backend_prepared_lookup_helper` already covers the
  prepared positive named/immediate/stack rows, unsupported move,
  missing-edge-publication status, Route 5 positive records, empty index,
  duplicate records, wrong predecessor, type mismatch, missing source, and
  missing destination. `backend_aarch64_current_block_join_routing` covers the
  Route 5 source reader that consumed the adjacent semantic identity in idea
  211. `backend_prepared_printer`,
  `backend_prealloc_block_entry_publications`,
  `backend_prepare_authoritative_join_ownership`,
  `backend_x86_prepared_handoff_label_authority`, and
  `backend_riscv_prepared_edge_publication` are byte-stability and wrapper
  guard surfaces.
- Missing proof gaps for Step 2/3: explicit route/prepared disagreement for
  this selected helper row, memory-source fallback on the selected row,
  no-source fallback that proves the prepared row is retained rather than a
  Route 5 status being surfaced, and a byte-stability run showing joined-branch,
  printer, x86 wrapper, and riscv wrapper output unchanged.

## Proof

Analysis-only Step 1 selection packet. No build or test was required and no
`test_after.log` was produced. Verified the recommended CTest targets exist
with `ctest --test-dir build -N`.
