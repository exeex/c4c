# Current Packet

Status: Active
Source Idea Path: ideas/open/839_lir_nominal_function_signature_call_composition.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate declarations to signature refs

## Just Finished

- Step 2 definition-printer slice completed: `lir_printer` now renders
  declarations and definitions with `LirFunctionSignatureRef` store facts for
  return type, return extension ABI, fixed params, variadic state, and void-list
  state. Definition rendering now takes parameter names from structured
  signature params and preserves retained header suffix attributes while
  ignoring stale `signature_text` return/parameter spelling. Nearby coverage
  now asserts stale definition `signature_text` cannot control printed
  signature types once a store ref exists.

## Suggested Next

- Continue Step 2 with the next bounded declaration/definition producer or
  remaining consumer migration through `LirFunctionSignatureRef`; do not move
  into call composition yet.

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
