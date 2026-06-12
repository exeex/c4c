Status: Active
Source Idea Path: ideas/open/230_phase_e3_route4_block_entry_publication_printer_debug_follow_up.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove Negative Cases and Nearby Same-Feature Stability

# Current Packet

## Just Finished

Step 2 - Thread Route 4 Attribution Under Prepared Agreement: completed the
printer-side threading for the selected available-register
`block_entry_publication` row.

- `src/backend/prealloc/prepared_printer/value_locations.cpp` now maps each
  prepared block-entry publication row to the matching BIR function, successor
  block, and leading PHI destination when available, then calls the existing
  `find_prepared_current_block_entry_publication(...)` agreement boundary.
- The printer only consumes the returned `PreparedCurrentBlockEntryPublication`
  copy when prepared lookup status is `Available`,
  `route4_block_entry_publication_attributed` is true, and the returned
  publication points back to the same prepared bundle and move. Otherwise it
  prints the original prepared-only row.
- Row spelling, section names, order, and the selected expected text stayed
  byte-stable:
  `block_entry_publication successor=join status=available to_value_id=42 to=published home_kind=register destination_kind=value destination_storage=register reg=r9 block_index=3 instruction_index=5`.
- `tests/backend/bir/backend_prepared_printer_test.cpp` now proves the selected
  row remains unchanged for prepared-only fallback, positive agreeing Route 4
  attribution at instruction index `5`, wrong-instruction fallback, and
  duplicate-reference fallback. Existing lookup tests continue to cover the
  wider absent/wrong/mismatched/prepared-only agreement matrix.

## Suggested Next

Delegate Step 3: tighten printer-level negative coverage for any remaining
same-feature fallback variants the supervisor wants visible at the row path
(for example missing PHI/wrong destination if not satisfied by existing lookup
coverage), and keep all expected row strings and CLI required snippets stable.

## Watchouts

- The implementation intentionally does not add route/debug words to the row;
  attribution is consumed only as an internal agreement path before printing
  prepared-owned text.
- The printer fallback for no BIR function/block/PHI is prepared-only because
  `route4_successor_block` or `route4_destination_value` is null in the query.
- The selected row fixture now has Route 4 variants in the prepared-printer
  test. Avoid turning those variants into row-text expectations beyond the
  existing byte-stable row.
- Wrapper/CLI no-change surfaces still need supervisor-selected Step 4 proof.
  Keep the `backend_cli_dump_prepared_bir_exposes_contract_sections` required
  snippet in `tests/backend/bir/CMakeLists.txt` stable:
  `block_entry_publication successor=logic.end.4 status=available to_value_id=9 to=%t8 home_kind=register destination_kind=value destination_storage=register reg=r11`.

## Proof

Command run for this implementation packet:

```bash
cmake --build build --target backend_prepared_printer_test backend_prealloc_block_entry_publications_test backend_prepared_lookup_helper_test && ctest --test-dir build -R '^(backend_prepared_printer|backend_prealloc_block_entry_publications|backend_prepared_lookup_helper)$' --output-on-failure > test_after.log 2>&1
```

Result: passed. `test_after.log` contains the green CTest run for
`backend_prepared_printer`, `backend_prepared_lookup_helper`, and
`backend_prealloc_block_entry_publications`.

Supervisor-run prepared CLI/printer no-change validation:

```bash
cmake --build build --target backend_prepared_printer_test && ctest --test-dir build -R '^(backend_prepared_printer|backend_cli_dump_prepared_bir.*)$' --output-on-failure
```

Result: passed 12/12.
