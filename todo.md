Status: Active
Source Idea Path: ideas/open/210_route4_block_entry_publication_printer_debug_row.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Add Route-Native Evidence for the Selected Row

# Current Packet

## Just Finished

Step 2 added Route 4 attribution metadata for the selected prepared-printer
block-entry publication row:

`block_entry_publication successor=<label> status=available to_value_id=<id> to=<value> home_kind=register destination_kind=value destination_storage=register reg=<register> block_index=<block_index> instruction_index=<instruction_index>`

`PreparedCurrentBlockEntryPublicationQueryInputs` now accepts optional
`route4_successor_block` and `route4_destination_value` evidence. The lookup
sets `route4_block_entry_publication_attributed` only when the selected
prepared available row agrees with a valid Route 4 block-entry publication
reference, the prepared successor label, prepared destination value name id,
destination value name/type, and PHI instruction index. Missing Route 4
evidence and mismatched destination evidence preserve the existing prepared
available row without attribution.

The prepared-printer row spelling and wrapper behavior were not changed.
`backend_prealloc_block_entry_publications` now covers no-route fallback,
positive Route 4 attribution, missing Route 4 PHI fallback, and mismatched
Route 4 destination fallback for this row.

## Suggested Next

Execute Step 3 from `plan.md`: add the remaining fail-closed diagnostics for
wrong successor/value references, duplicate or ambiguous Route 4 facts where
observable, and route/prepared mismatch without changing row text.

## Watchouts

- Keep subsequent work on the same available-register
  `block_entry_publication` row class.
- The current attribution helper combines Route 4 index validation with a
  direct Route 4 record carrying the prepared name id; preserve both checks so
  duplicate/stale index failures remain fail-closed while prepared identity can
  be compared.
- Do not add a printed attribution column or rewrite expected row strings for
  this route.
- Step 3 should focus on negative proof gaps, especially duplicate/ambiguous
  Route 4 evidence and explicit route/prepared mismatch fallback.

## Proof

Passed:
`cmake --build --preset default --target backend_prealloc_block_entry_publications_test backend_prepared_printer_test backend_x86_publication_plan_reuse_test && ctest --test-dir build -j --output-on-failure -R '^(backend_prealloc_block_entry_publications|backend_prepared_printer|backend_x86_publication_plan_reuse)$' > test_after.log`

`test_after.log` records 3/3 passing:
`backend_prealloc_block_entry_publications`, `backend_prepared_printer`, and
`backend_x86_publication_plan_reuse`.
