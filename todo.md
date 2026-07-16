# Current Packet

Status: Active
Source Idea Path: ideas/open/838_lir_canonical_module_owned_aggregate_ref_store_convergence.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Preserve recursive aggregate facts and all aggregate forms

## Just Finished

- Step 2 migrated the bounded `lir_owned_type_spec` function-signature
  occurrence producer to consume populated HIR `QualType::aggregate_ref`
  through the LIR module aggregate store relation. Type declaration lowering now
  registers the HIR definition's existing canonical ref when present, so
  ordinary aggregate return/parameter occurrences resolve through the same
  store entry instead of issuing a fresh definition-side ref. The legacy
  no-owner compatibility path remains limited to occurrences without canonical
  carriers, while populated refs fail closed on missing store entries,
  incoherent union/struct kind, malformed LIR names, or corrupted retained HIR
  owner identity.

## Suggested Next

- Continue Step 2 with one bounded downstream consumer/proof packet that checks
  the next function-signature aggregate consumer still reads the canonical LIR
  aggregate store facts and does not fall back to tag/text/owner-key
  reconstruction; keep verifier/printer/backend migrations out unless the
  selected consumer strictly requires them.

## Watchouts

- `lir_owned_type_spec` still keeps the explicit no-owner compatibility return
  for fixtures without canonical carriers. Do not expand that into a
  reconstruction path. Populated refs must resolve through
  `LirModule::find_aggregate_ref` / `find_aggregate`, and complete but
  unmatched legacy owner metadata must continue to fail closed.
- Do not widen into unrelated consumer, verifier/printer, backend, 836, or 831
  work.

## Proof

- Passed:
  `( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_function_signature_type_ref|frontend_hir_tests)$' ) > test_after.log 2>&1`
- Supervisor checkpoint passed:
  `( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' ) > /tmp/c4c_backend_after.log 2>&1`
- Proof log: `test_after.log`.
