Status: Active
Source Idea Path: ideas/open/212_route5_edge_join_oracle_printer_row.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Prove Fail-Closed Edge/Join Diagnostics

# Current Packet

## Just Finished

Step 3 added focused fail-closed proof for the selected
`PreparedCurrentBlockJoinParallelCopySourceFact` named current-block join row.
`backend_prepared_lookup_helper` now checks that absent Route 5 source data,
explicit no-source evidence, source mismatch, destination type mismatch,
memory-source evidence, duplicate agreeing records, and wrong-predecessor
evidence all retain the prepared available row without attaching a Route 5
record.

The same test also keeps the adjacent immediate, stack/source-value, and
unsupported-move facts prepared-owned, so the proof remains limited to the one
selected helper-oracle row and does not move branch, parallel-copy, move-bundle,
printer, wrapper, or expected-string ownership into Route 5.

## Suggested Next

Execute Step 4 from `plan.md`: preserve printer, wrapper, and joined-branch
strings. The next packet should run or strengthen the selected expected-string
and wrapper no-change checks without changing row text or wrapper output.

Recommended proof command:

```bash
cmake --build --preset default --target backend_prepared_lookup_helper_test backend_prepared_printer_test backend_prealloc_block_entry_publications_test backend_aarch64_current_block_join_routing_test backend_prepare_authoritative_join_ownership_test backend_x86_prepared_handoff_label_authority_test backend_riscv_prepared_edge_publication_test && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_prepared_printer|backend_prealloc_block_entry_publications|backend_aarch64_current_block_join_routing|backend_prepare_authoritative_join_ownership|backend_x86_prepared_handoff_label_authority|backend_riscv_prepared_edge_publication)$' > test_after.log
```

## Watchouts

- Keep Step 4 to byte-stability and wrapper/joined-branch proof for the selected
  row. Do not migrate the
  whole edge-publication, source-producer, move-bundle, joined-branch, wrapper,
  or printer families.
- The implementation intentionally exposes Route 5 as helper-oracle metadata
  only; prepared edge publication, move-bundle, branch, parallel-copy, printer,
  and wrapper ownership remain unchanged.
- Step 3 uses manually cloned Route 5 indexes to exercise bad evidence while
  keeping prepared facts constant. The empty-index helper status is
  `MissingSuccessor`; wrong-predecessor evidence is rejected by the agreement
  gate after the finder reports an available destination/source record.
- `clang-format` is not installed in this environment, so no formatter was run.

## Proof

Step 3 proof passed with the exact delegated command:

```bash
cmake --build --preset default --target backend_prepared_lookup_helper_test backend_prepared_printer_test backend_prealloc_block_entry_publications_test backend_aarch64_current_block_join_routing_test backend_prepare_authoritative_join_ownership_test backend_x86_prepared_handoff_label_authority_test backend_riscv_prepared_edge_publication_test && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_prepared_printer|backend_prealloc_block_entry_publications|backend_aarch64_current_block_join_routing|backend_prepare_authoritative_join_ownership|backend_x86_prepared_handoff_label_authority|backend_riscv_prepared_edge_publication)$' > test_after.log
```

`test_after.log` records 7/7 passing:
`backend_prepared_lookup_helper`, `backend_aarch64_current_block_join_routing`,
`backend_riscv_prepared_edge_publication`, `backend_prepared_printer`,
`backend_prealloc_block_entry_publications`,
`backend_x86_prepared_handoff_label_authority`, and
`backend_prepare_authoritative_join_ownership`.
