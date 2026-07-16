# Current Packet

Status: Active
Source Idea Path: ideas/open/839_lir_nominal_function_signature_call_composition.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate declarations to signature refs

## Just Finished

- Step 1 established the initial module-owned nominal function-signature store:
  `LirFunctionSignatureRef`, store entries for return/ordered parameter type
  refs, byval flags, variadic state, and void-list state, lowering-time
  registration from existing structured function signature facts, and verifier
  checks that stored facts match the function's structured mirrors exactly.

## Suggested Next

- Continue Step 1 with a bounded declaration/call construction probe if needed,
  or move to Step 2 by migrating declarations to consume
  `LirFunctionSignatureRef` while keeping compatibility mirrors intact.

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

## Proof

- Passed focused proof plus shared LIR backend checkpoint in
  `test_after.log`:
  `( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_function_signature_type_ref|frontend_lir_call_type_ref|frontend_hir_tests)$' && ctest --test-dir build -j --output-on-failure -R '^backend_lir_' ) > test_after.log 2>&1`
