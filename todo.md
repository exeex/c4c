# Current Packet

Status: Active
Source Idea Path: ideas/open/838_lir_canonical_module_owned_aggregate_ref_store_convergence.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Enforce module ownership and migrate bounded consumers

## Just Finished

- Step 3 verifier consumer packet completed. Direct aggregate
  function-signature return and parameter mirrors that carry `StructNameId`
  now validate against canonical `LirModule::aggregate_store` facts when the
  module has canonical aggregate-store state. The verifier rejects missing
  matching store entries and incoherent struct/union/layout facts while
  preserving no-owner compatibility and byval/non-aggregate behavior.

## Suggested Next

- Continue Step 3 with one bounded printer or receiver consumer that can read
  canonical LIR aggregate store facts directly, with focused valid/invalid
  proof and no expansion into backend or broad nominal-family cleanup.

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

## Proof

- Passed:
  `( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_function_signature_type_ref|frontend_hir_tests)$' ) > test_after.log 2>&1`
- Proof log: `test_after.log`.
