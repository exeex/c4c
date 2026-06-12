Status: Active
Source Idea Path: ideas/open/212_route5_edge_join_oracle_printer_row.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Preserve Printer, Wrapper, and Joined-Branch Strings

# Current Packet

## Just Finished

Step 4 preserved the printer, wrapper, and joined-branch string surfaces for the
selected `PreparedCurrentBlockJoinParallelCopySourceFact` named current-block
join row without changing expected strings.

Existing tests were sufficient for this packet: `backend_prepared_printer`
keeps the adjacent prepared-printer parallel-copy and block-entry publication
rows byte-stable, `backend_prepare_authoritative_join_ownership` keeps
joined-branch and parallel-copy ownership prepared-authored,
`backend_x86_prepared_handoff_label_authority` compares the exact x86 wrapper
stub output, `backend_riscv_prepared_edge_publication` keeps target-local edge
publication instruction text stable, and the aarch64/helper-oracle tests keep
the selected Route 5 row bounded to helper metadata.

## Suggested Next

Execute Step 5 from `plan.md`: acceptance review. The next packet should compare
the final diff against the source idea and verify no broad edge-publication,
source-producer, move-bundle, joined-branch, wrapper, or printer-family
migration slipped in.

## Watchouts

- Step 4 did not add new assertions because the existing expected-string and
  wrapper tests already covered the required surfaces for this selected
  helper-oracle row.
- Keep Step 5 to acceptance review. Do not migrate the whole edge-publication,
  source-producer, move-bundle, joined-branch, wrapper, or printer families.
- The implementation intentionally exposes Route 5 as helper-oracle metadata
  only; prepared edge publication, move-bundle, branch, parallel-copy, printer,
  and wrapper ownership remain unchanged.
- `clang-format` is not installed in this environment, so no formatter was run.

## Proof

Step 4 proof passed with the exact delegated command:

```bash
cmake --build --preset default --target backend_prepared_lookup_helper_test backend_prepared_printer_test backend_prealloc_block_entry_publications_test backend_aarch64_current_block_join_routing_test backend_prepare_authoritative_join_ownership_test backend_x86_prepared_handoff_label_authority_test backend_riscv_prepared_edge_publication_test && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_prepared_printer|backend_prealloc_block_entry_publications|backend_aarch64_current_block_join_routing|backend_prepare_authoritative_join_ownership|backend_x86_prepared_handoff_label_authority|backend_riscv_prepared_edge_publication)$' > test_after.log
```

`test_after.log` records 7/7 passing:
`backend_prepared_lookup_helper`, `backend_aarch64_current_block_join_routing`,
`backend_riscv_prepared_edge_publication`, `backend_prepared_printer`,
`backend_prealloc_block_entry_publications`,
`backend_x86_prepared_handoff_label_authority`, and
`backend_prepare_authoritative_join_ownership`.
