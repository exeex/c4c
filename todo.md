# Current Packet

Status: Active
Source Idea Path: ideas/open/839_lir_nominal_function_signature_call_composition.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Delete migrated mirrors and assess convergence

## Just Finished

- Completed Step 5 deletion-gate slice: migrated store-backed fixed direct-call
  verifier/import consumers no longer depend on duplicate `arg_type_refs` for
  argument type authority; they validate through `LirFunctionSignatureRef` and
  `LirCallArg::type_ref`. Added stale duplicate `arg_type_refs` coverage that
  proves store-backed direct integer calls ignore the mirror while structured
  argument/store mismatches still fail through existing checks.

## Suggested Next

- Return Step 5 to supervisor/plan-owner for convergence assessment: remaining
  mirror deletion gates need an owner decision because their named consumers
  still include legacy/raw or later-idea authority paths.

## Watchouts

- `signature_text` still has named final-output/header compatibility users in
  the printer, verifier, and aggregate/back-end ABI paths for legacy/no-store
  functions; deleting the field is not yet an in-scope Step 5 code deletion.
- `args_str` and parsed-call construction still have named raw compatibility,
  inline-asm, and legacy call parsing/rewrite users. Removing them would cross
  raw adapter or later body/argument identity ownership.
- The `arg_type_refs` field still has named verifier/import/native intrinsic
  consumers outside the migrated fixed direct-call slice; this packet deleted
  only the migrated consumer dependency on that duplicate mirror.
- Do not absorb 829/830 body-use or argument identity, scalar/vector/union
  migration, or generic value-carrier work.
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
