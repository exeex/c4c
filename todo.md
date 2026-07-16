# Current Packet

Status: Active
Source Idea Path: ideas/open/839_lir_nominal_function_signature_call_composition.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate call composition to signature refs

## Just Finished

- Step 3 BIR store-backed integer intrinsic slice completed: the compiled
  monolithic `exact_integer_intrinsic_call` consumer now resolves
  `callee_signature_ref` through the module function signature store before
  retained `callee_signature`, permits retained signature absence or text-stale
  mirrors when the store entry resolves, and keeps retained/store, store/call,
  and argument signature mismatches fail-closed for the bounded
  `cttz`/`ctlz`/`ctpop` integer intrinsic shapes. This repair added explicit
  nearby coverage for store-backed one-parameter `ctpop` with retained
  `callee_signature` absent or retained text stale, plus a store/call parameter
  mismatch rejection, without changing argument value identity.

## Suggested Next

- Continue Step 3 with the next remaining call-composition consumer outside the
  migrated direct void, direct integer, native floating, and integer intrinsic
  call-authority helpers, without touching 829/830 argument value identity.

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

## Proof

- Passed delegated proof in `test_after.log`:
  `( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_function_signature_type_ref|frontend_lir_call_type_ref|frontend_hir_tests)$' && ctest --test-dir build -j --output-on-failure -R '^backend_lir_' ) > test_after.log 2>&1`
