# Current Packet

Status: Active
Source Idea Path: ideas/open/839_lir_nominal_function_signature_call_composition.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate call composition to signature refs

## Just Finished

- Step 3 fixed direct-call signature-ref slice completed: `LirCallOp` now has
  an opt-in `callee_signature_ref`, supported non-extern/non-variadic/
  non-unspecified/non-byval direct calls populate it from the resolved callee
  function's module-owned `LirFunctionSignatureRef`, and verifier checks prove
  the call ref names the same module function/store signature as the retained
  structured call signature. Nearby coverage now proves a fixed aggregate
  direct call carries the nominal ref, stale or missing refs reject for that
  supported shape, and no-prototype direct calls remain on compatibility
  metadata.

## Suggested Next

- Continue Step 3 with the next bounded call-composition slice: either migrate
  another supported direct-call signature shape through `callee_signature_ref`
  or move one verifier/printer/reference consumer from retained
  `callee_signature`/`args_str` compatibility toward the nominal ref. Keep
  extern declarations, no-prototype, variadic, byval, and 829/830 argument
  value identity as explicit later gates unless the packet owns one of them.

## Watchouts

- Do not absorb 829/830 body-use or argument identity, scalar/vector/union
  migration, or generic value-carrier work.
- Do not derive semantic signatures from rendered text, `signature_text`,
  `args_str`, parsed call strings, duplicate `arg_type_refs`, or testcase
  spelling.
- Preserve accepted aggregate owner checks from 838; wrong-module aggregate
  alternatives must fail closed.
- `LirTypeRef` semantic equality can ignore stale rendered text when a
  `StructNameId` matches; signature-store verifier checks must continue using
  exact stored fact agreement where mirrors are still retained.
- RV64 variadic return `signext`/`zeroext` is now a structured
  `signature_return_ext_attr` fact and must stay in the store path.

## Proof

- Passed focused proof plus shared LIR backend checkpoint in
  `test_after.log`:
  `( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_function_signature_type_ref|frontend_lir_call_type_ref|frontend_hir_tests)$' && ctest --test-dir build -j --output-on-failure -R '^backend_lir_' ) > test_after.log 2>&1`
