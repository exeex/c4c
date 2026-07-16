# Current Packet

Status: Active
Source Idea Path: ideas/open/839_lir_nominal_function_signature_call_composition.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate call composition to signature refs

## Just Finished

- Step 3 direct-call printer consumer slice completed: for calls with a valid
  `callee_signature_ref`, `lir_printer` now renders the direct callee type
  suffix from the module-owned function signature store instead of retained
  `callee_type_suffix` text. Nearby coverage mutates the retained direct-call
  suffix and proves printing still uses the store-backed fixed aggregate
  signature while stale suffix text is ignored.

## Suggested Next

- Continue Step 3 with the next bounded call-composition slice: either migrate
  another supported direct-call signature shape through `callee_signature_ref`
  or move one remaining verifier/reference consumer from retained
  `callee_signature`/`args_str` compatibility toward the nominal ref. Keep
  extern declarations, no-prototype, variadic, byval, indirect calls, and
  829/830 argument value identity as explicit later gates unless the packet owns
  one of them.

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
