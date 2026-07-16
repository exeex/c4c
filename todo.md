# Current Packet

Status: Active
Source Idea Path: ideas/open/838_lir_canonical_module_owned_aggregate_ref_store_convergence.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Preserve recursive aggregate facts and all aggregate forms

## Just Finished

- Step 2 added focused fail-closed coverage for the migrated
  `lir_owned_type_spec` function-signature occurrence producer. Ordinary
  aggregate return and parameter occurrences with populated but corrupted HIR
  `QualType::aggregate_ref` facts now prove rejection before any owner-key,
  tag, or rendered-text compatibility can repair them: stale module-owned refs,
  foreign/wrong-module refs, and parameter-side stale refs all fail with the
  registered-ref boundary. Production code was unchanged because the current
  store lookup already enforces the boundary.

## Suggested Next

- Treat Step 2 as ready for supervisor acceptance and move to Step 3 activation
  if no additional Step 2 acceptance gap is identified. The next coherent
  packet is the first bounded consumer migration/proof under Step 3, selected
  from declaration, field, call, verifier, printer, or receiver consumers that
  can read canonical LIR aggregate store facts without tag/text/owner-key
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
