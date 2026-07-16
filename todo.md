# Current Packet

Status: Active
Source Idea Path: ideas/open/838_lir_canonical_module_owned_aggregate_ref_store_convergence.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Enforce module ownership and migrate bounded consumers

## Just Finished

- Step 3 receiver consumer packet completed. The LIR-to-BIR structured
  spelling/layout receiver now traverses canonical
  `LirModule::aggregate_store` entries when the store is populated, validates
  each recorded declaration fact against the matching structured declaration,
  and rejects missing or incoherent store facts before falling back to
  declaration-only authority. Empty aggregate-store modules retain the existing
  no-owner compatibility path.

## Suggested Next

- Continue Step 3 with one bounded remaining consumer that still reads aggregate
  identity from legacy declaration/text authority, or move to Step 4 if
  supervisor review finds the named Step 3 consumer set exhausted.

## Watchouts

- `lir_owned_type_spec` still keeps the explicit no-owner compatibility return
  for fixtures without canonical carriers. Do not expand that into a
  reconstruction path. Populated refs must resolve through
  `LirModule::find_aggregate_ref` / `find_aggregate`, and complete but
  unmatched legacy owner metadata must continue to fail closed. The new
  corrupted-ref tests intentionally keep valid rendered/tag metadata available
  so the rejection proves the populated-ref path does not fall through to the
  compatibility branch.
- The verifier packet intentionally treats an empty canonical aggregate store
  as the legacy no-owner compatibility boundary. Corruption tests remove or
  mutate only the relevant `Pair` store facts while other canonical aggregate
  facts remain, so rejection proves the direct signature verifier is not using
  `StructNameId`, tag, or rendered text alone as authority.
- Do not widen into unrelated consumer, verifier/printer, backend, 836, or 831
  work.
- Printer declaration rendering now uses aggregate-store traversal only when
  `aggregate_store` is nonempty. Keep this as a consumption path, not a
  reconstruction path: missing, stale, or incoherent store facts should reject
  through verification rather than recovering identity from `struct_decls`,
  rendered text, or declaration order.
- The public backend interface proof exercises the consolidated
  `src/backend/bir/lir_to_bir.cpp` importer in addition to the split
  `lir_to_bir/types.cpp` and `module.cpp` receiver seam. Keep any follow-up
  receiver packets aligned across both import paths until one path is retired.

## Proof

- Passed:
  `( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_interface|frontend_lir_function_signature_type_ref|frontend_hir_tests)$' ) > test_after.log 2>&1`
- Proof log: `test_after.log`.
