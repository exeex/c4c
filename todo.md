Status: Active
Source Idea Path: ideas/open/208_route3_memory_source_oracle_printer_row.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Select The Diagnostic Row And Baseline Surface

# Current Packet

## Just Finished

Completed `plan.md` Step 1 by selecting exactly one diagnostic row for later implementation: helper-oracle status row `Route 3 load-local stored-value source matches prepared for exact same-slot ranges`, exposed by `verify_route3_load_local_stored_value_source_matches_prepared_or_falls_back()` in `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`.

Selected row type: helper-oracle status row, not a prepared addressing printer row.

Prepared fallback/status matrix for the selected row:
- Positive: prepared `find_prepared_same_block_load_local_stored_value_source(...)` and Route 3 `find_bir_same_block_load_local_stored_value_source_identity(...)` both identify the same earlier `StoreLocal` source value for the same `LoadLocal` result, same local slot, same byte range, and same before-instruction boundary.
- Absent/no-memory: missing Route 3 facts, missing BIR block, missing named root value, missing load-local producer, or missing same-slot store must not invent a route-native row; prepared lookup remains the oracle/fallback.
- Invalid-reference: wrong block label, void/unnamed root value, out-of-range before-index clamping, non-`LoadLocal` root producer, non-`StoreLocal` source, or malformed memory access record must fail closed to the current prepared helper behavior.
- Duplicate/conflict: same-slot stores with overlapping but non-identical byte ranges, missing stored-value identity, or ambiguous source candidates must reject route-native authority and keep prepared behavior.
- Mismatch: route/prepared disagreement on load instruction index, store instruction index, stored value name/type, local slot identity, byte range, or before-instruction boundary must preserve the prepared status instead of accepting Route 3.
- Fallback/target-addressing: frame slot offsets, size/align, `can_use_base_plus_offset`, stack-layout object identity, and final address/operand legality remain prepared/AArch64 policy; Route 3 may only provide target-neutral stored-value source identity.

Route 3 evidence source to inspect in Step 2: `bir::route3_find_same_block_load_local_stored_value_source(...)` in `src/backend/bir/bir.cpp`, the MIR adapter `mir::find_bir_same_block_load_local_stored_value_source_identity(...)` in `src/backend/mir/query.cpp`, and the existing AArch64 prepared-fallback consumer `find_indirect_callee_stored_value_source(...)` in `src/backend/mir/aarch64/codegen/calls.cpp`.

Expected byte-stable surfaces: the helper-oracle failure strings in `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`, the `backend_prepared_lookup_helper` test name, and prepared addressing dump text under `--- prepared-addressing ---` must remain unchanged. No prepared printer row or expected snippet should be refreshed for this row-selection slice.

## Suggested Next

Execute `plan.md` Step 2 for the selected helper-oracle row by adding the smallest route/prepared agreement helper needed around the stored-value source identity row, while keeping prepared fallback authoritative for absent, invalid, duplicate/conflict, mismatch, and target-addressing-policy cases.

## Watchouts

- Keep the slice to one diagnostic row; do not migrate whole memory oracle families, printer sections, target wrappers, public fallback, aggregate lookup behavior, or `PreparedFunctionLookups`.
- Preserve prepared status behavior for absent, non-memory, invalid, duplicate/conflict, alias/address conflict, mismatch, fallback, and target-policy-sensitive paths.
- Do not refresh baselines, weaken statuses, rename helpers, rewrite expected strings, migrate wrappers, remove prepared fallback, or move target-addressing policy into Route 3.
- The selected row already has Route 3 production evidence in the indirect-callee stored-value source path; Step 2 should treat that as evidence/fallback context, not as permission to broaden into AArch64 call lowering, address formation, or target-wrapper migration.

## Proof

Analysis-only selection packet; no build or test was required and no `test_after.log` was produced. Recommended later narrow proof command after implementation: `cmake --build build --target backend_prepared_lookup_helper_test && ctest --test-dir build -R '^backend_prepared_lookup_helper$' --output-on-failure`, with byte-stability checked against the selected helper-oracle strings and any nearby prepared-addressing dump snippets touched by the diff.
