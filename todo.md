Status: Active
Source Idea Path: ideas/open/210_route4_block_entry_publication_printer_debug_row.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Name the Row and Baseline Prepared Output

# Current Packet

## Just Finished

Step 1 selected one exact prepared-printer row class for Route 4 block-entry
publication diagnostics:

`block_entry_publication successor=<label> status=available to_value_id=<id> to=<value> home_kind=register destination_kind=value destination_storage=register reg=<register> block_index=<block_index> instruction_index=<instruction_index>`

The row is emitted by `append_value_locations(...)` in
`src/backend/prealloc/prepared_printer/value_locations.cpp` under
`--- prepared-block-entry-publications ---`. It is prepared value-location
printer output, not publication-plan output or a route-debug oracle row.

Baseline prepared semantics for the selected row:
- Prepared authority comes from `collect_prepared_block_entry_publications(...)`
  and `find_prepared_current_block_entry_publication(...)`.
- A positive row requires block-entry move phase, out-of-SSA parallel-copy
  authority, matching successor label, destination kind `value`, register
  destination storage, destination value-home identity, and a register spelling
  from the move or register value-home fallback.
- Prepared fallback remains authoritative for missing value locations, invalid
  successor labels, missing destination homes, missing publications,
  publication-unavailable statuses, unsupported destination storage/kind,
  unsupported move operation, and missing register name.
- Route 4 evidence source for later implementation is
  `mir::find_bir_block_entry_publication_identity(...)`, which builds Route 4
  publication availability from the successor BIR block and validates
  `route_index_validate_block_entry_publication_reference(...)`.
- Route evidence must only attribute the selected row when it agrees with the
  prepared destination value id/name/type, successor identity, PHI destination
  instruction index, destination home, destination kind/storage, and register
  spelling. Any absent, stale, wrong-key, wrong-relationship, duplicate,
  diverged, or route/prepared-mismatched fact must fall back to the prepared
  row or prepared absence status.

Existing coverage found:
- `backend_prealloc_block_entry_publications` covers positive available
  register publication, register value-home fallback, unsupported stack
  storage, missing register name, missing destination home, unsupported
  destination kind, unsupported operation, wrong/unmatched successor, missing
  value locations, invalid successor, missing publication, BIR block-entry
  identity agreement, BIR-only PHI identity not implying prepared readiness,
  wrong successor, and wrong destination value.
- `backend_x86_publication_plan_reuse` covers shared wrapper reuse and
  incomplete publication-plan rejection for x86 consumers.

Missing or weak proof surface for Step 2/3:
- No direct `backend_prepared_printer` expected-string assertion was found for
  the selected `block_entry_publication` row text.
- Duplicate/ambiguous Route 4 block-entry publication evidence is available at
  the route validation layer, but this selected row still needs an explicit
  prepared-printer or helper-oracle fail-closed assertion tying duplicate route
  evidence back to prepared fallback.
- The later implementation packet should add route/prepared mismatch proof
  without rewriting this row text or weakening existing prepared expectations.

## Suggested Next

Execute Step 2 from `plan.md`: add route-native attribution for the selected
available register `block_entry_publication` row behind strict
route/prepared-agreement checks while keeping prepared output and fallback
semantics byte-stable.

## Watchouts

- Keep the packet to this one row class only; do not migrate wrapper families,
  CLI dump sections, broad prepared-printer surfaces, publication mechanics, or
  target output policy.
- Do not rewrite expected strings as evidence. If a string assertion is added,
  it should freeze the current row text.
- Treat duplicate/ambiguous Route 4 facts, wrong successor, wrong destination,
  stale/diverged records, route/prepared mismatch, missing route evidence, and
  prepared fallback as required proof dimensions.
- Wrapper no-change should be proved if shared publication helpers or public
  publication-plan surfaces are touched.

## Proof

Analysis-only packet; no build required and no `test_after.log` produced.
Recommended narrow proof command for the implementation packet:
`cmake --build --preset default --target backend_prealloc_block_entry_publications_test backend_prepared_printer_test backend_x86_publication_plan_reuse_test && ctest --test-dir build -R '^(backend_prealloc_block_entry_publications|backend_prepared_printer|backend_x86_publication_plan_reuse)$' --output-on-failure | tee test_after.log`
