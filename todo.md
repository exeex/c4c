Status: Active
Source Idea Path: ideas/open/229_phase_e3_route3_memory_source_stored_value_helper_oracle_follow_up.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Validate Route 3 Agreement And Fail-Closed Boundaries

# Current Packet

## Just Finished

Step 1 discovery completed for the selected Route 3 memory/source
stored-value helper-oracle row.

Selected row:

- `backend_prepared_lookup_helper` row:
  `verify_route3_load_local_stored_value_source_matches_prepared_or_falls_back`
  in `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`.
- Positive row shape: same-block `StoreLocal %stored` / `LoadLocal %loaded`
  over the same local slot, byte range, and type; prepared success comes from
  `prepare::find_prepared_same_block_load_local_stored_value_source(...)`.
- Production consumer row to keep aligned: selected indirect-callee stored
  source in `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`,
  fixture `prepared_with_selected_stored_indirect_callee(...)`, where
  `StoreLocal %callee.selected` at instruction 3 feeds `LoadLocal
  %callee.loaded` at instruction 4 before the indirect call at instruction 5.

Route 3 evidence path:

- `src/backend/mir/query.cpp`:
  `mir::find_bir_same_block_load_local_stored_value_source_identity(...)`
  builds a Route 3 memory access index and reads
  `bir::route3_find_same_block_load_local_stored_value_source(...)`.
- Route 3 evidence must be `LoadLocal` + `StoreLocal` over `LocalSlot`, with
  matching root value name/type, loaded value, stored value, load/store
  instruction indexes, local slot id, byte offset, and byte size.
- Production Route 3 read is currently
  `find_route3_indirect_callee_stored_value_source_identity(...)` in
  `src/backend/mir/aarch64/codegen/calls.cpp`, selected by
  `find_indirect_callee_stored_value_source(...)` before prepared fallback.

Prepared fallback authority:

- `src/backend/prealloc/prepared_lookups.cpp`:
  `prepare::find_prepared_same_block_load_local_stored_value_source(...)`
  remains the fallback/oracle for prepared source-producer facts, prepared
  memory access lookup, frame-slot identity, overlapping range ambiguity, exact
  range match, and target-policy-sensitive prepared address fields.
- `src/backend/mir/aarch64/codegen/calls.cpp`:
  `find_prepared_indirect_callee_stored_value_source_fallback(...)` remains the
  production fallback when Route 3 evidence is absent, invalid, mismatched, or
  not authoritative for policy-sensitive behavior.

Target files for Step 2 inspection/proof:

- Implementation/readers: `src/backend/mir/query.cpp`,
  `src/backend/bir/bir.cpp`, `src/backend/bir/bir.hpp`,
  `src/backend/prealloc/prepared_lookups.cpp`,
  `src/backend/prealloc/prepared_lookups.hpp`,
  `src/backend/mir/aarch64/codegen/calls.cpp`.
- Tests: `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`,
  `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`,
  `tests/backend/mir/backend_aarch64_prepared_memory_operand_records_test.cpp`,
  `tests/backend/mir/backend_aarch64_memory_operand_records_test.cpp`.
- CTest names: `backend_prepared_lookup_helper`,
  `backend_aarch64_instruction_dispatch`,
  `backend_aarch64_prepared_memory_operand_records`, and
  `backend_aarch64_memory_operand_records`.

Positive/fallback coverage found:

- Positive helper-oracle coverage exists for exact same-slot stored-value
  agreement in `verify_route3_load_local_stored_value_source_matches_prepared_or_falls_back`.
- Invalid Route 3 references are covered by wrong block label and void root
  type in the helper row.
- Absent/invalid Route 3 range authority is covered by zero-sized BIR memory
  ranges while prepared lookup remains available.
- Prepared/Route 3 mismatch is covered by changing prepared store byte offset
  while Route 3 still sees the BIR same-slot fact.
- Alias/address ambiguity is covered by an overlapping conflicting same-slot
  store that must fail closed under prepared authority.
- Target-policy retention is covered by changing prepared
  `can_use_base_plus_offset` and confirming prepared policy remains outside
  Route 3 source identity.
- Nearby same-feature Route 3 memory/source coverage exists in
  `backend_aarch64_prepared_memory_operand_records_test.cpp` and
  `backend_aarch64_memory_operand_records_test.cpp` for memory access records,
  stored-value records, load/store local/global records, node/base mismatch,
  missing prepared access, result/stored identity mismatch, and address fact
  mismatch.
- Production positive/fallback coverage exists in
  `block_dispatch_uses_route3_stored_indirect_callee_identity_for_selected_source`
  and
  `block_dispatch_falls_back_to_prepared_stored_indirect_callee_policy`.
- Adjacent non-agreement fallback coverage exists in
  `block_dispatch_indirect_callee_route4_agreement_preserves_prepared_policy`
  for missing, mismatched, and wrong-reference Route 4 source-producer data
  around the same indirect-callee selected source path.

## Suggested Next

Step 2 - Validate Route 3 Agreement And Fail-Closed Boundaries. Confirm that
the selected helper-oracle row and production indirect-callee row require an
explicit Route 3/prepared agreement gate, or record that the existing
Route 3-first plus prepared-fallback split is already sufficient for the
selected row without moving target-addressing policy.

## Watchouts

Proof gaps for Step 2:

- Non-memory negative coverage for this exact stored-value helper row is not
  explicit yet; current Route 3 stored-value evidence requires local-slot
  `LoadLocal`/`StoreLocal`, but Step 2 should decide whether an explicit
  non-memory helper-row assertion is needed.
- Prepared/Route 3 mismatch coverage exists for prepared byte-offset mismatch;
  Step 2 should verify whether slot-id, stored-value name/type, result name,
  and instruction-index mismatch paths need focused assertions before any
  behavior change.
- Production fallback coverage proves missing Route 3 range authority and one
  mismatched prepared store slot through dispatch output, but it does not yet
  isolate every absent, invalid, ambiguous, and non-agreement helper-oracle
  status at the selected row.
- Nearby memory/source tests cover Route 3 record mechanics, but Step 2 should
  name the exact subset that prevents this from becoming a named-fixture
  shortcut.
- Do not change helper-oracle strings, wrappers, expected strings, prepared
  addressing printer output, address formation, materialization, addressing
  legality, final operands, target policy, baselines, or supported-path
  contracts.

## Proof

Delegated proof for this discovery-only packet:
`git diff --check -- todo.md && git status --short`.

No build or test subset was delegated for Step 1. The delegated proof does not
produce `test_after.log`; no alternate log was created.
