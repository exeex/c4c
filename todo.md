# Current Packet

Status: Active
Source Idea Path: ideas/open/839_lir_nominal_function_signature_call_composition.md
Source Plan Path: plan.md
Current Step ID: 3A
Current Step Title: Repair variadic declaration admission before call migration

## Just Finished

- Step 3A completed: backend LIR variadic declarations now pass function
  admission only when declaration mirrors match module-owned
  `LirFunctionSignatureRef` store facts, including structured variadic state,
  fixed-prefix refs/byval facts, and return extension attributes. Nearby
  backend coverage proves a variadic direct-call fixture reaches
  store-backed `callee_signature_ref` validation, while retained/store and
  declaration/store variadic disagreement fail closed without making retained
  text or parsed call strings authoritative.

## Suggested Next

- Continue repaired Step 3 at Step 3B: add the narrow raw extern
  signature-store adapter entries needed for direct-call compatibility through
  `LirFunctionSignatureRef`, without absorbing broader extern/global type-fact
  migration.

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
- Step 3B may add a raw extern one-way adapter for function-signature facts, but
  must not absorb the broader global/extern type-fact migration owned by later
  work such as 844.
- Step 3C may split store-backed signature authority from retained
  verifier-local ABI metadata, but must return to plan-owner if it requires
  aggregate producer migration, body parameter authority, or unrestricted value
  carrier work.

## Proof

- Passed delegated proof in `test_after.log`:
  `( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_function_signature_type_ref|frontend_lir_call_type_ref|frontend_hir_tests)$' && ctest --test-dir build -j --output-on-failure -R '^backend_lir_' ) > test_after.log 2>&1`
