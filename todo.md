# Current Packet

Status: Active
Source Idea Path: ideas/open/838_lir_canonical_module_owned_aggregate_ref_store_convergence.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Enforce module ownership and migrate bounded consumers

## Just Finished

- Step 2 is accepted through `88ccf591d`. The bounded
  `lir_owned_type_spec` function-signature producer consumes populated HIR
  `QualType::aggregate_ref` through the LIR aggregate store, and focused proof
  now covers both valid ordinary aggregate signatures and stale, foreign, and
  parameter-side corrupted populated refs failing closed before owner-key,
  tag, or rendered-text compatibility can repair them.

## Suggested Next

- Start Step 3 with one bounded consumer migration/proof packet selected from
  declaration, field, call, verifier, printer, or receiver consumers that can
  read canonical LIR aggregate store facts without tag/text/owner-key
  reconstruction.

## Watchouts

- `lir_owned_type_spec` still keeps the explicit no-owner compatibility return
  for fixtures without canonical carriers. Do not expand that into a
  reconstruction path. Populated refs must resolve through
  `LirModule::find_aggregate_ref` / `find_aggregate`, and complete but
  unmatched legacy owner metadata must continue to fail closed. The new
  corrupted-ref tests intentionally keep valid rendered/tag metadata available
  so the rejection proves the populated-ref path does not fall through to the
  compatibility branch.
- Do not widen into unrelated consumer, verifier/printer, backend, 836, or 831
  work.

## Proof

- Passed:
  `( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_function_signature_type_ref|frontend_hir_tests)$' ) > test_after.log 2>&1`
- Supervisor checkpoint passed:
  `( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' ) > /tmp/c4c_backend_after.log 2>&1`
- Proof log: `test_after.log`.
