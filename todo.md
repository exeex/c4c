Status: Active
Source Idea Path: ideas/open/229_phase_e3_route3_memory_source_stored_value_helper_oracle_follow_up.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Wire Or Prove The Selected Helper-Oracle Row

# Current Packet

## Just Finished

Step 2 validation completed for the selected Route 3 memory/source
stored-value helper-oracle row.

Agreement gate found:

- The selected helper row,
  `verify_route3_load_local_stored_value_source_matches_prepared_or_falls_back`,
  has an explicit prepared/Route 3 agreement check through
  `prepared_and_bir_same_block_load_local_stored_source_match(...)`.
- The positive helper row requires prepared and Route 3 to agree on the
  load-local producer, stored value, store/load instruction indexes, local-slot
  memory kind, local slot id, byte offset, byte size, root value, loaded value,
  and stored-value type/name.
- `mir::find_bir_same_block_load_local_stored_value_source_identity(...)` is
  target-neutral Route 3 source identity only. It rejects wrong block labels,
  void roots, missing memory records, non-local-slot memory, non-LoadLocal /
  non-StoreLocal evidence, zero-sized ranges, overlapping same-slot conflicts,
  and missing stored-value records before returning identity.

Fallback matrix validated:

- Absent Route 3 evidence: zero-sized BIR memory ranges make Route 3 unavailable
  while `prepare::find_prepared_same_block_load_local_stored_value_source(...)`
  remains available; production fallback dispatch also passes when BIR range
  authority is erased.
- Invalid Route 3 evidence: wrong block label and void root type fail closed in
  the helper row; production falls through to prepared when Route 3 returns
  `std::nullopt`.
- Ambiguous evidence: overlapping same-slot stores reject the helper agreement
  path under prepared authority.
- Mismatched evidence: prepared byte-offset mismatch rejects helper agreement
  even when Route 3 can still see the same-slot BIR fact.
- Non-memory evidence: the Route 3 stored-value identity reader cannot return
  without a local-slot `LoadLocal` and preceding local-slot `StoreLocal`; nearby
  memory-record tests cover unsupported or mismatched memory facts failing
  closed.
- Non-agreement evidence: helper agreement is fail-closed when prepared and
  Route 3 availability or identity differs.

Semantic gap for Step 3:

- The production AArch64 indirect-callee path in
  `src/backend/mir/aarch64/codegen/calls.cpp` does not use the helper-row
  agreement gate. `find_indirect_callee_stored_value_source(...)` accepts
  `find_route3_indirect_callee_stored_value_source_identity(...)` before
  consulting `find_prepared_indirect_callee_stored_value_source_fallback(...)`.
- Existing dispatch coverage proves the gap:
  `block_dispatch_uses_route3_stored_indirect_callee_identity_for_selected_source`
  calls `prepared_with_selected_stored_indirect_callee(false, true)`, where
  Route 3 is present but the prepared store slot is deliberately mismatched,
  and the production path still uses Route 3 selected-source identity.
- Target-addressing, materialization, addressing legality, and final operands
  remain prepared-authoritative: Route 3 only supplies stored source identity
  and store index, while prepared still owns call-plan callee register,
  target-policy fields, direct-global dependency checks, address formation,
  materialization, legality, and emitted final operands.

## Suggested Next

Step 3 - Wire Or Prove The Selected Helper-Oracle Row. Add a narrow production
agreement gate for the AArch64 indirect-callee stored-value source path, or
route that consumer through an equivalent prepared/Route 3 agreement helper,
so Route 3 is used only when prepared same-block load-local stored-value source
behavior agrees.

## Watchouts

- Do not preserve the current production Route 3-first behavior for
  `prepared_with_selected_stored_indirect_callee(false, true)` as success
  unless the supervisor changes source intent; under the active plan it is the
  concrete non-agreement gap.
- Step 3 should add or adjust focused tests for production prepared/Route 3
  mismatch fallback. The helper row already covers prepared byte-offset
  mismatch, invalid references, absent Route 3 range authority, ambiguity, and
  target-policy separation.
- Consider an explicit non-memory assertion for the selected helper row only if
  Step 3 needs extra proof beyond the current Route 3 local-slot
  LoadLocal/StoreLocal construction and nearby memory-record fail-closed tests.
- Do not change helper-oracle strings, wrappers, expected strings, prepared
  addressing printer output, address formation, materialization, addressing
  legality, final operands, target policy, baselines, or supported-path
  contracts.

## Proof

Delegated proof passed:
`cmake --build build --target backend_prepared_lookup_helper_test backend_aarch64_instruction_dispatch_test backend_aarch64_prepared_memory_operand_records_test backend_aarch64_memory_operand_records_test && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch|backend_aarch64_prepared_memory_operand_records|backend_aarch64_memory_operand_records)$' --output-on-failure > test_after.log 2>&1`.

`test_after.log` contains 4/4 passing tests:
`backend_prepared_lookup_helper`, `backend_aarch64_instruction_dispatch`,
`backend_aarch64_prepared_memory_operand_records`, and
`backend_aarch64_memory_operand_records`.
