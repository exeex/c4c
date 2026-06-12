Status: Active
Source Idea Path: ideas/open/230_phase_e3_route4_block_entry_publication_printer_debug_follow_up.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Thread Route 4 Attribution Under Prepared Agreement

# Current Packet

## Just Finished

Step 1 - Locate the Exact Row and Agreement Boundary: completed discovery for
the Route 4 block-entry publication printer/debug follow-up.

Exact selected row:

```text
block_entry_publication successor=join status=available to_value_id=42 to=published home_kind=register destination_kind=value destination_storage=register reg=r9 block_index=3 instruction_index=5
```

Source row fixture and assertion:

- `tests/backend/bir/backend_prepared_printer_test.cpp`: `prepared_block_entry_publication_printer_row_module()` builds the row with function
  `block_entry_publication_dump_contract`, successor `join`, value
  `published`, `PreparedValueId{42}`, register home/move `r9`, block index
  `3`, and instruction index `5`.
- `src/backend/prealloc/prepared_printer/value_locations.cpp` prints this row
  from `collect_prepared_block_entry_publications(...)` inside
  `append_value_locations(...)`.

Chosen agreement boundary:

- Reuse the existing `find_prepared_current_block_entry_publication(...)` /
  `attribute_route4_block_entry_publication_if_agreeing(...)` boundary in
  `src/backend/prealloc/prepared_lookups.cpp`.
- Route 4 attribution may be trusted only when the prepared lookup is
  `PreparedCurrentBlockEntryPublicationStatus::Available`,
  `prepared_block_entry_publication_available(result.publication)` is true,
  `route4_successor_block` and `route4_destination_value` are present, the
  successor label and prepared destination identity are valid, and the prepared
  publication has a bundle.
- The agreeing Route 4 reference must validate through
  `route_index_validate_block_entry_publication_reference(...)`; the resulting
  Route 4 record must agree on successor label id, destination prepared value
  name id, destination value name/type, destination value type, and prepared
  bundle instruction index before
  `route4_block_entry_publication_attributed` is set.

Minimal Step 2 implementation target:

- Primary: `src/backend/prealloc/prepared_printer/value_locations.cpp`.
- Existing metadata is already available in `PreparedCurrentBlockEntryPublication`
  (`route4_block_entry_publication_attributed`,
  `route4_block_entry_publication_status`,
  `route4_block_entry_publication_route_status`, and
  `route4_block_entry_publication_instruction_index`), so no
  `src/backend/prealloc/value_locations.hpp` or
  `src/backend/prealloc/prepared_lookups.cpp` change should be needed unless
  the printer cannot locally map a prepared row to the matching BIR successor
  block and destination PHI value.

## Suggested Next

Delegate Step 2: in `src/backend/prealloc/prepared_printer/value_locations.cpp`,
route the selected available-register `block_entry_publication` row through the
existing `find_prepared_current_block_entry_publication(...)` agreement
boundary when the prepared module can supply the matching BIR successor block
and destination PHI value; print the same prepared row text and fail closed to
the current prepared-only row for absent, missing, duplicate, wrong-key,
wrong-successor, mismatched, or otherwise disagreeing Route 4 evidence.

## Watchouts

- Do not add route/debug words to the row or change section order. The selected
  row must remain byte-stable and still spell
  `block_entry_publication successor=... status=available ... destination_storage=register`.
- `append_value_locations(...)` currently has only
  `PreparedBlockEntryPublication` rows from
  `collect_prepared_block_entry_publications(...)`; those rows do not carry
  Route 4 attribution. Step 2 must consume the existing
  `PreparedCurrentBlockEntryPublication` result or build a narrow local query
  path, not create a parallel attribution policy.
- Positive proof case already exists for the lookup boundary in
  `tests/backend/bir/backend_prealloc_block_entry_publications_test.cpp`:
  `%published` with prepared register publication `r9` and an agreeing Route 4
  PHI sets `route4_block_entry_publication_attributed`, `Valid`,
  `Available`, and instruction index `0`.
- Negative lookup cases already exist for no Route 4 evidence, destination
  mismatch, wrong successor, wrong type/key, duplicate reference,
  stack-destination publication-unavailable, BIR-only PHI, missing publication,
  wrong prepared successor, and wrong destination value.
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` also covers the
  Route 4 facade/direct-reference boundary for missing publication, missing
  value, wrong key, stale owner, wrong relationship, duplicate reference, and
  diverged record.
- Proof gaps before acceptance: no prepared-printer test currently proves that
  this exact printed row used Route 4 attribution under agreement; the
  `prepared_block_entry_publication_printer_row_module()` fixture currently
  supplies value-location data only and no BIR successor PHI for Route 4
  evidence; there are no printer-level negative/fallback cases proving the row
  remains unchanged for absent/wrong/duplicate/mismatched Route 4 evidence.
- Wrapper/CLI no-change surfaces: keep
  `backend_prepared_printer` row text stable and keep the
  `backend_cli_dump_prepared_bir_exposes_contract_sections` required snippet in
  `tests/backend/bir/CMakeLists.txt` stable:
  `block_entry_publication successor=logic.end.4 status=available to_value_id=9 to=%t8 home_kind=register destination_kind=value destination_storage=register reg=r11`.
  The scan found that x86 CLI prepared dump snippet as the only existing
  explicit CLI `block_entry_publication` row contract; additional
  x86/riscv/AArch64 wrapper-visible no-change proof still needs supervisor
  selection for Step 4.

## Proof

Command run for this discovery-only packet:

```bash
git diff --check -- todo.md && git status --short
```

Result: passed; `git status --short` reports only `M todo.md`. No code, tests,
source idea, baselines, expected strings, wrappers, CLI dump output, or
implementation files were changed.
