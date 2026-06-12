Status: Active
Source Idea Path: ideas/open/210_route4_block_entry_publication_printer_debug_row.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Prove Fail-Closed Diagnostics

# Current Packet

## Just Finished

Step 3 made the remaining fail-closed diagnostics observable for the selected
prepared-printer block-entry publication row:

`block_entry_publication successor=<label> status=available to_value_id=<id> to=<value> home_kind=register destination_kind=value destination_storage=register reg=<register> block_index=<block_index> instruction_index=<instruction_index>`

`backend_prealloc_block_entry_publications` now proves that Route 4 evidence
falls closed without attribution while preserving prepared availability for
wrong successor identity, wrong value/type key, and duplicate block-entry
publication facts. Existing Step 2 coverage still proves no-route fallback,
positive Route 4 attribution, missing Route 4 PHI fallback, and mismatched
destination fallback.

No selected row spelling, prepared-printer expected string, wrapper behavior,
or production lookup behavior was changed in this packet.

## Suggested Next

Execute Step 4 from `plan.md`: preserve row text and wrapper stability by
re-running the selected prepared-printer/dump string proof and wrapper proof
without changing expected strings.

## Watchouts

- Keep subsequent work on the same available-register
  `block_entry_publication` row class.
- The duplicate case is observable through
  `RouteIndexValidationStatus::DuplicateReference`; there is no separate
  ambiguous status for this selected Route 4 block-entry row.
- Step 4 should be proof-only unless a missing wrapper/string check requires
  a narrow test registration update.
- Do not add a printed attribution column or rewrite expected row strings.

## Proof

Passed:
`cmake --build --preset default --target backend_prealloc_block_entry_publications_test backend_prepared_printer_test backend_x86_publication_plan_reuse_test && ctest --test-dir build -j --output-on-failure -R '^(backend_prealloc_block_entry_publications|backend_prepared_printer|backend_x86_publication_plan_reuse)$' > test_after.log`

`test_after.log` records 3/3 passing:
`backend_prealloc_block_entry_publications`, `backend_prepared_printer`, and
`backend_x86_publication_plan_reuse`.
