Status: Active
Source Idea Path: ideas/open/230_phase_e3_route4_block_entry_publication_printer_debug_follow_up.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Wrapper and Broader No-Change Proof

# Current Packet

## Just Finished

Step 3 - Prove Negative Cases and Nearby Same-Feature Stability: tightened the
prepared-printer proof matrix for the selected available-register
`block_entry_publication` row without changing row text.

- `tests/backend/bir/backend_prepared_printer_test.cpp` now proves the selected
  row remains unchanged for prepared-only fallback, positive agreeing Route 4
  attribution at instruction index `5`, missing-PHI fallback,
  wrong-destination fallback, wrong-successor fallback, wrong-instruction
  fallback, and duplicate-reference fallback.
- The selected expected row stayed byte-stable:
  `block_entry_publication successor=join status=available to_value_id=42 to=published home_kind=register destination_kind=value destination_storage=register reg=r9 block_index=3 instruction_index=5`.
- Nearby same-feature stability is covered by the unchanged
  `backend_prepared_lookup_helper` and
  `backend_prealloc_block_entry_publications` subset, including the existing
  wider missing, mismatched destination, wrong successor, wrong key/type, and
  duplicate Route 4 agreement matrix.

## Suggested Next

Delegate Step 4: run the supervisor-selected wrapper/CLI no-change proof for
the selected row and required snippets, with no expected-string or row-text
changes.

## Watchouts

- The printer fallback variants intentionally assert the same prepared-owned row
  spelling; do not add Route 4/debug wording to printed rows.
- Missing-PHI, wrong-destination, and wrong-successor printer variants fall back
  before attribution because the row path cannot assemble both a matching
  successor block and matching destination PHI for the prepared publication.
- Wrapper/CLI no-change surfaces still need supervisor-selected Step 4 proof.
  Keep the `backend_cli_dump_prepared_bir_exposes_contract_sections` required
  snippet in `tests/backend/bir/CMakeLists.txt` stable:
  `block_entry_publication successor=logic.end.4 status=available to_value_id=9 to=%t8 home_kind=register destination_kind=value destination_storage=register reg=r11`.

## Proof

Command run for this Step 3 proof packet:

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
