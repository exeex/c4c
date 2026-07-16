# Current Packet

Status: Active
Source Idea Path: ideas/open/839_lir_nominal_function_signature_call_composition.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate call composition to signature refs

## Just Finished

- Step 3 integer intrinsic call-authority verifier slice completed: the
  boolean-flag (`cttz`/`ctlz`) and count (`ctpop`) direct-call helpers now
  consume the module-owned function signature store when `callee_signature_ref`
  resolves, with retained `callee_signature` left as the compatibility fallback
  for unmigrated intrinsic calls. Nearby coverage proves supported boolean-flag
  and count intrinsic direct calls verify with retained `callee_signature`
  removed once a valid signature ref is present, while stale retained structured
  signature facts still reject through the signature-ref disagreement check.

## Suggested Next

- Continue Step 3 with another direct-call composition consumer outside the
  migrated direct void fixed-integer and integer intrinsic call-authority
  helpers, without touching 829/830 argument value identity.

## Watchouts

- Do not absorb 829/830 body-use or argument identity, scalar/vector/union
  migration, or generic value-carrier work.
- Do not derive semantic signatures from rendered text, `signature_text`,
  `args_str`, parsed call strings, duplicate `arg_type_refs`, or testcase
  spelling.
- Preserve accepted aggregate owner checks from 838; wrong-module aggregate
  alternatives must fail closed.
- `verify_call_callee_signature_ref` still rejects retained structured signature
  disagreement with the store; this packet only made retained text/mirror
  presence non-authoritative for the migrated verifier consumer.
- RV64 variadic return `signext`/`zeroext` is now a structured
  `signature_return_ext_attr` fact and must stay in the store path.

## Proof

- Passed delegated proof in `test_after.log`:
  `( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_function_signature_type_ref|frontend_lir_call_type_ref|frontend_hir_tests)$' && ctest --test-dir build -j --output-on-failure -R '^backend_lir_' ) > test_after.log 2>&1`
