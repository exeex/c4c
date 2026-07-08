Status: Active
Source Idea Path: ideas/open/589_direct_edge_publication_move_freshness_ownership.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Strengthen Observability And Closure Inventory

# Current Packet

## Just Finished

Step 5 added prepared-printer observability for the migrated current-block join
parallel-copy source route. The new
`prepared-current-block-join-parallel-copy-sources` dump rows are computed from
`prepare_current_block_join_parallel_copy_source_facts` and expose each fact's
edge/source status, source freshness query status, candidate count, selected
authority source kind, source value/id, use, proof, rank, reference
block/instruction, source/destination home classes, immediate-source flag, and
Route 5 agreement status.

Closure inventory recorded for the direct edge-publication freshness route:
the migrated family is the current-block join parallel-copy source route
through `prepare_block_entry_parallel_copy_edge_source_facts` plus
`prepare_current_block_join_parallel_copy_source_facts`. Protected families are
object traversal move-bundle consumers, which remain protected by
`MoveBundleSource`, and dependency/store-source producer-publication consumers,
which remain protected by existing 587/588 freshness surfaces. Deferred
families are the row publisher/helper
`make_prepared_edge_publication_lookups`, broad RV64/AArch64/x86 emission
tails, and non-selected edge-publication or move-bundle consumer families.
Typed stack-source and aggregate stack-source publication routes are blocked on
missing producer/publication facts. Select-carrier alias, destination fan-in,
predecessor-edge consumed suppression, and similar destination/alias legality
routes are blocked on ownership design and are split-worthy if continued. No
new target-specific gap was found in this packet; any follow-up idea should
focus on select/alias or typed/aggregate stack-source freshness only if the
supervisor chooses to continue beyond this representative route.

## Suggested Next

Proceed to Step 6 with freshness regression-anchor proof for the migrated
direct edge-publication route and existing 587/588 freshness surfaces.

## Watchouts

- The dump section intentionally prints only rows that can be derived through
  the prepared semantic lookup path; it does not fabricate invalid or ambiguous
  test-only authority states.
- Missing, invalid, ambiguous, wrong-value, wrong-use, and destination-only
  fail-closed freshness statuses remain covered by focused helper tests; the
  printer adds selected/no-candidate visibility for the migrated route.
- Immediate edge sources remain authority-free and should not be retrofitted
  with freshness candidates.

## Proof

Focused pre-proof passed:
`cmake --build --preset default --target backend_prepared_printer_test backend_prepared_lookup_helper_test && ctest --test-dir build -j --output-on-failure -R 'backend_prepared_(printer|lookup_helper)'`.

Delegated proof passed:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`.
Proof log: `test_after.log`.
