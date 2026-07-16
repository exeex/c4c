# Current Packet

Status: Active
Source Idea Path: ideas/open/839_lir_nominal_function_signature_call_composition.md
Source Plan Path: plan.md
Current Step ID: 3C
Current Step Title: Split byval store authority from retained ABI metadata

## Just Finished

- Step 3C completed: fixed aggregate/byval direct-call composition now uses
  module `LirFunctionSignatureRef` store facts for parameter authority while
  retained ABI-shaped byval metadata remains only a verifier/import
  compatibility check for existing argument/layout handling. Backend LIR
  coverage proves store-backed byval calls import with and without retained
  `callee_signature`, while retained/store disagreement, store/call parameter
  mismatch, and wrong aggregate alternatives fail closed.

## Suggested Next

- Continue repaired Step 3 toward Step 4 readiness: migrate printer/reference
  collector observation of nominal function-signature facts without restoring
  rendered signature text as semantic authority.

## Watchouts

- Do not absorb 829/830 body-use or argument identity, scalar/vector/union
  migration, or generic value-carrier work.
- Do not derive semantic signatures from rendered text, `signature_text`,
  `args_str`, parsed call strings, duplicate `arg_type_refs`, or testcase
  spelling.
- Native integer intrinsics still deliberately lower to BIR intrinsic call
  payloads carrying link-name identity, not ordinary direct `CallNode` targets;
  future packets should keep that distinction unless their source idea says
  otherwise.
- The fixed direct-call argument-authority migration is still only the consumer
  check for an existing single argument0 authority row; it does not expand body
  parameter production or argument value identity.
- Preserve accepted aggregate owner checks from 838; wrong-module aggregate
  alternatives must fail closed.
- `verify_call_callee_signature_ref` still rejects retained structured signature
  disagreement with the store; this packet only made retained text/mirror
  presence non-authoritative for the migrated verifier consumer.
- RV64 variadic return `signext`/`zeroext` is now a structured
  `signature_return_ext_attr` fact and must stay in the store path.
- The ineffective split-file-only patch in
  `src/backend/bir/lir_to_bir/calling.cpp` was removed for this slice; the
  compiled backend interface path is the monolithic importer in
  `src/backend/bir/lir_to_bir.cpp`.
- Step 3A admitted only variadic declaration/callee signature facts. Variadic
  body-use, va_list lowering, variadic argument value identity, and runtime
  helper work remain out of scope under source 839.
- The raw extern adapter is intentionally narrow: it creates function-signature
  facts for LinkNameId-backed extern function declarations and direct-call
  verification/import, not broader global/extern initializer or type-fact
  migration.
- Byval call support remains limited to existing selected pointer/global
  argument routes and declaration/import signature composition; aggregate
  producer migration, body parameter authority, and unrestricted value carrier
  work remain out of scope.

## Proof

- Passed delegated proof in `test_after.log`:
  `( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_function_signature_type_ref|frontend_lir_call_type_ref|frontend_hir_tests)$' && ctest --test-dir build -j --output-on-failure -R '^backend_lir_' ) > test_after.log 2>&1`
