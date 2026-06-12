Status: Active
Source Idea Path: ideas/open/210_route4_block_entry_publication_printer_debug_row.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Preserve Row Text and Wrapper Stability

# Current Packet

## Just Finished

Step 4 preserved the selected prepared-printer block-entry publication row text
and wrapper behavior.

`block_entry_publication successor=<label> status=available to_value_id=<id> to=<value> home_kind=register destination_kind=value destination_storage=register reg=<register> block_index=<block_index> instruction_index=<instruction_index>`

`backend_prepared_printer` now freezes the concrete current row spelling:

`block_entry_publication successor=join status=available to_value_id=42 to=published home_kind=register destination_kind=value destination_storage=register reg=r9 block_index=3 instruction_index=5`

`backend_x86_publication_plan_reuse` remains the wrapper no-change proof for
the shared publication-plan consumer surface. No production lookup behavior,
wrapper logic, CLI section names, or existing expected strings were rewritten.

## Suggested Next

Execute Step 5 from `plan.md`: acceptance review against the source idea,
checking for route drift, expectation weakening, wrapper-family expansion,
prepared API contraction, and aggregate migration.

## Watchouts

- Keep review on the same available-register
  `block_entry_publication` row class.
- The duplicate case is observable through
  `RouteIndexValidationStatus::DuplicateReference`; there is no separate
  ambiguous status for this selected Route 4 block-entry row.
- The new prepared-printer assertion is a narrow byte-stability freeze for the
  selected row text, not an expected-string refresh.
- Do not add a printed attribution column or rewrite expected row strings.

## Proof

Passed:
`cmake --build --preset default --target backend_prealloc_block_entry_publications_test backend_prepared_printer_test backend_x86_publication_plan_reuse_test && ctest --test-dir build -j --output-on-failure -R '^(backend_prealloc_block_entry_publications|backend_prepared_printer|backend_x86_publication_plan_reuse)$' > test_after.log`

`test_after.log` records 3/3 passing:
`backend_prealloc_block_entry_publications`, `backend_prepared_printer`, and
`backend_x86_publication_plan_reuse`.
