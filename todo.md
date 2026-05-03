# Current Packet

Status: Active
Source Idea Path: ideas/open/142_codegen_lir_aggregate_type_identity_metadata.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Probe TypeSpec Tag Removal Boundary

## Just Finished

Step 5 - Probe TypeSpec Tag Removal Boundary:
migrated the `src/codegen/lir/hir_to_lir/lvalue.cpp` member-field access
boundary off direct `access.base_ts.tag` fallback. Member access now resolves
the owner tag from structured `TypeSpec` owner metadata first, then uses the
existing named aggregate compatibility helper as the legacy route.

## Suggested Next

Next coherent packet: rerun the temporary `TypeSpec::tag` deletion probe and
migrate the next field-chain boundary in `src/codegen/lir/hir_to_lir/types.cpp`,
where `lookup_field_chain_layout` and anonymous/base field recursion still
assign or read `TypeSpec::tag`.

## Watchouts

- The probe edit was temporary: `const char* tag` in
  `src/frontend/parser/ast.hpp` was restored before the post-probe rebuild.
- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp` no longer contains `.tag` reads;
  remaining `struct_defs.find(...)` calls in that file use declaration-order or
  base-tag strings, not `TypeSpec::tag`.
- `lvalue.cpp` no longer reads `access.base_ts.tag`; the remaining known
  deletion-probe residuals are in the `types.cpp` field-chain helpers unless a
  fresh probe exposes an earlier boundary.

## Proof

Proof command:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_lir_.*|cpp_hir_(sema_canonical_symbol|sema_consteval_type_utils).*structured_metadata|cpp_positive_sema_(c_style_cast_.*field_access|inherited_base_member_access_runtime|inherited_base_aggregate_init_runtime|record_nested_aggregate_member_parse|operator_struct_byval_param|struct_method|template_struct.*)_cpp|positive_sema_ok_call_variadic_aggregate_runtime_c|abi_abi_variadic_struct_result_c|llvm_gcc_c_torture_src_(pta_field_[12]|struct_(aliasing_1|cpy_1|ini_[1-4]|ret_2)|zero_struct_[12])_c|eastl_cpp_external_utility_frontend_basic_cpp)$"' > test_after.log 2>&1`
passed. `test_after.log` now contains the accepted focused proof: 38/38 tests
passed after a successful default preset build.
