# Current Packet

Status: Active
Source Idea Path: ideas/open/820_lir_directscalar_parameter_producer_verifier_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish and verify the typed DirectScalar authority

## Just Finished

- Step 2 completed the typed DirectScalar producer/verifier repair. `init_fn_ctx`
  now publishes the body-parameter definition with its existing typed signature
  mirror, rather than reconstructing a parallel rendered scalar type.
- The verifier admits only the producer's bounded direct-scalar structural
  family and still requires matching logical/signature bases, an exact typed
  mirror, valid current-function value identity, and current-function owner.
  Source alias/qualification metadata remains non-authoritative.
- `frontend_lir_function_signature_type_ref_test` now covers a typedef-backed
  `ull` DirectScalar authority plus missing/invalid value identity, foreign
  owner, and type-incoherent typed-mirror rejection. The gcc-torture case
  passes before any Raw-BIR receipt changes.

## Suggested Next

- Advance to the next supervisor-selected Step 3 packet; retain this packet's
  DirectScalar boundary and do not broaden it into generic scalar receipt.

## Watchouts

- No Raw-BIR/importer files were changed. Unselected DirectScalar rows remain
  outside Raw-BIR receipt.
- The selected scalar-LHS verifier uses the same bounded DirectScalar admission
  predicate so source aliases cannot make a valid `ull` identity disappear
  downstream.

## Proof

- Focused LIR authority test passed:
  `ctest --test-dir build --output-on-failure -R '^frontend_lir_function_signature_type_ref$'`.
- Supervisor-selected proof passed:
  `cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^llvm_gcc_c_torture_src_20041011_1_c$' 2>&1 | tee test_after.log`.
  Canonical proof log: `test_after.log`.
