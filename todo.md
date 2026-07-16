# Current Packet

Status: Active
Source Idea Path: ideas/open/838_lir_canonical_module_owned_aggregate_ref_store_convergence.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Enforce module ownership and migrate bounded consumers

## Just Finished

- Step 3 baseline repair packet completed after the rejected baseline candidate
  following `97eb154af`. The repair restored legitimate non-store structured
  declaration emission/import while preserving canonical aggregate-store
  authority, so the variadic/`va_arg` cases no longer produce unsized
  `%struct.__va_list_tag_` GEP bases. It also restored the bounded legacy
  template-specialization owner bridge needed by
  `cpp_positive_sema_template_inline_method_member_context_frontend_cpp`, while
  populated HIR aggregate refs still resolve through
  `LirModule::find_aggregate_ref` / `find_aggregate` and corrupted refs still
  fail closed.
- Baseline repair continuation then fixed the remaining representative
  aggregate owner/signature failures from the fresh full-suite candidate:
  namespace aggregates, nested template aggregate signature mirrors, anonymous
  GCC torture aggregates, and quoted EASTL template-specialization names now
  resolve through bounded declaration-backed compatibility without making
  tag/text lookup durable authority.
- A fresh full-suite baseline candidate after `74e17efe3` still regressed by
  10 tests. The follow-up repair closes that remaining family by making direct
  signature and call aggregate mirrors prefer existing module declaration/store
  layout facts over stale legacy struct/union bits, while preserving
  no-declaration global-header compatibility and keeping explicit non-root
  stale owner metadata rejected.
- Supervisor accepted the fresh full-suite baseline review at `53a1a8515`: the
  full suite reported 3038/3038 passing with no failure-set expansion.

## Suggested Next

- Resume normal Step 3 consumer migration or Step 4 assessment from `plan.md`.

## Watchouts

- `lir_owned_type_spec` still keeps the explicit no-owner compatibility return
  for fixtures without canonical carriers. Do not expand that into a
  reconstruction path. Populated refs must resolve through
  `LirModule::find_aggregate_ref` / `find_aggregate`, and complete but
  unmatched legacy owner metadata must continue to fail closed. The new
  corrupted-ref tests intentionally keep valid rendered/tag metadata available
  so the rejection proves the populated-ref path does not fall through to the
  compatibility branch.
- The verifier packet intentionally treats an empty canonical aggregate store
  as the legacy no-owner compatibility boundary. Corruption tests remove or
  mutate only the relevant `Pair` store facts while other canonical aggregate
  facts remain, so rejection proves the direct signature verifier is not using
  `StructNameId`, tag, or rendered text alone as authority.
- Do not widen into unrelated consumer, verifier/printer, backend, 836, or 831
  work.
- Printer declaration rendering now uses aggregate-store traversal only when
  `aggregate_store` is nonempty. Keep this as a consumption path, not a
  reconstruction path: missing, stale, or incoherent store facts should reject
  through verification rather than recovering identity from `struct_decls`,
  rendered text, or declaration order.
- The public backend interface proof exercises the consolidated
  `src/backend/bir/lir_to_bir.cpp` importer in addition to the split
  `lir_to_bir/types.cpp` and `module.cpp` receiver seam. Keep any follow-up
  receiver packets aligned across both import paths until one path is retired.
- Call argument mirrors with complete structured call authority skip rendered
  argument parsing, so their aggregate-store validation must stay independent
  of the text-parsing branch. The owned call test also repairs stale fixture
  metadata so populated aggregate refs fail closed while explicit no-owner
  compatibility clears those refs before lowering.
- Baseline repair must not weaken populated-ref fail-closed behavior, revive
  tag/text/key lookup as authority, or classify supported aggregate signatures
  as unsupported. Preserve legitimate no-owner structured declarations when the
  aggregate store is nonempty, and close only the bounded legacy-owner gap
  proven by the C++ inline-method member-context failure.
- The baseline expansion blocker is cleared. Do not reopen baseline repair
  unless a later fresh candidate expands the accepted 0-failure set.

## Proof

- Initial focused repro subset failed before the repair:
  `( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(positive_sema_ok_call_variadic_aggregate_runtime_c|cpp_positive_sema_template_inline_method_member_context_frontend_cpp|llvm_gcc_c_torture_src_va_arg_13_c)$' ) > /tmp/c4c_baseline_reject_probe.log 2>&1`
- Passed delegated repair proof:
  `( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(positive_sema_ok_call_variadic_aggregate_runtime_c|cpp_positive_sema_template_inline_method_member_context_frontend_cpp|llvm_gcc_c_torture_src_va_arg_13_c|frontend_lir_function_signature_type_ref|frontend_lir_call_type_ref|backend_lir_to_bir_interface|frontend_hir_tests)$' ) > test_after.log 2>&1`
- Supervisor checkpoint passed:
  `( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' ) > /tmp/c4c_backend_after.log 2>&1`
- Passed baseline-continuation proof:
  `( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(cpp_positive_sema_namespace_struct_runtime_cpp|cpp_positive_sema_template_struct_nested_cpp|llvm_gcc_c_torture_src_20071029_1_c|eastl_cpp_external_utility_frontend_basic_cpp|frontend_lir_function_signature_type_ref|frontend_lir_call_type_ref|frontend_hir_tests)$' ) > test_after.log 2>&1`
- Refreshed backend checkpoint passed:
  `( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' ) > /tmp/c4c_backend_after.log 2>&1`
- Rejected-baseline 10-test follow-up proof passed:
  `( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_function_signature_type_ref|c_testsuite_src_00220_c|llvm_gcc_c_torture_src_ieee_fp_cmp_1_c|llvm_gcc_c_torture_src_ieee_fp_cmp_2_c|llvm_gcc_c_torture_src_ieee_fp_cmp_3_c|llvm_gcc_c_torture_src_20180131_1_c|llvm_gcc_c_torture_src_921112_1_c|llvm_gcc_c_torture_src_921204_1_c|llvm_gcc_c_torture_src_930208_1_c|llvm_gcc_c_torture_src_bswap_2_c|llvm_gcc_c_torture_src_pr23324_c)$' ) > test_after.log 2>&1`
- Refreshed aggregate/signature proof and backend checkpoint passed after the
  follow-up repair:
  `( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(cpp_positive_sema_namespace_struct_runtime_cpp|cpp_positive_sema_template_struct_nested_cpp|llvm_gcc_c_torture_src_20071029_1_c|eastl_cpp_external_utility_frontend_basic_cpp|frontend_lir_function_signature_type_ref|frontend_lir_call_type_ref|frontend_hir_tests)$' ) > test_after.log 2>&1 && ( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' ) > /tmp/c4c_backend_after.log 2>&1`
- Accepted full-suite baseline review:
  `scripts/plan_review_state.py accept-baseline` after `test_baseline.new.log`
  reported `100% tests passed, 0 tests failed out of 3038`.
- Proof log: `test_after.log`.
