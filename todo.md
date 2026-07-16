# Current Packet

Status: Active
Source Idea Path: ideas/open/839_lir_nominal_function_signature_call_composition.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate call composition to signature refs

## Just Finished

- Step 3 direct-call verifier consumer slice completed: the fixed direct-call
  type-authority gate and `verify_call_callee_signature` now consume the
  module-owned function signature store when `callee_signature_ref` is valid,
  while retained `callee_signature` stays a compatibility shadow checked for
  disagreement. Nearby coverage proves an aggregate direct call verifies with
  the retained signature mirror removed and ignores stale retained parameter
  text while the signature ref names the store facts.

## Suggested Next

- Continue Step 3 with one remaining direct-call composition consumer that still
  reads retained call text or argument type mirrors, preferably the next verifier
  helper that can be moved to `callee_signature_ref` without touching 829/830
  argument value identity.

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

- Passed focused proof plus shared LIR backend checkpoint in `test_after.log`:
  `( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_function_signature_type_ref|frontend_lir_call_type_ref|frontend_hir_tests)$' && ctest --test-dir build -j --output-on-failure -R '^backend_lir_' ) > test_after.log 2>&1`
