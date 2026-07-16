# Current Packet

Status: Active
Source Idea Path: ideas/open/839_lir_nominal_function_signature_call_composition.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate call composition to signature refs

## Just Finished

- Step 2 is complete enough to advance: declarations and definitions now carry
  valid `LirFunctionSignatureRef` store refs; verifier checks compare stored
  return type, return extension ABI, fixed params, variadic state, void-list
  state, and byval facts against structured declaration/definition mirrors;
  printer rendering for declarations and definitions is store-backed and ignores
  stale `signature_text` spelling once a store ref exists. Nearby coverage
  includes zero-parameter/explicit-void, fixed, variadic, aggregate
  return/parameter, byval, stale structured mirror rejection, stale
  `signature_text` non-authority, and missing/mismatched aggregate name-id
  rejection. Recent accepted proof is recorded below.

## Suggested Next

- Begin Step 3 with the smallest bounded direct-call slice: identify supported
  direct-call construction paths that still derive semantic call signatures
  from parsed call text or `args_str`, route one fixed-signature call path
  through `LirFunctionSignatureRef`, and add nearby verifier/lowering coverage
  proving the call composes a legal store ref without claiming 829/830 argument
  value identity.

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
