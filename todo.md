# Current Packet

Status: Active
Source Idea Path: ideas/open/142_codegen_lir_aggregate_type_identity_metadata.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Probe TypeSpec Tag Removal Boundary

## Just Finished

Step 5 - Probe TypeSpec Tag Removal Boundary:
migrated the narrow member function-pointer signature discovery route in
`src/codegen/lir/hir_to_lir/expr/coordinator.cpp` off direct `base_ts.tag`
reads. The route now asks `find_typespec_aggregate_layout()` for the owner
layout, which prefers structured aggregate owner metadata and keeps the
rendered-name path behind the helper's explicitly named compatibility fallback.

## Suggested Next

Next coherent packet: run the temporary `TypeSpec::tag` deletion probe again
and migrate the next narrow `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`
boundary it exposes. Expected same-wave candidates from the prior probe are
LIR-owned `TypeSpec` tag interning, aggregate type-ref helpers, object
alignment, flexible-array global lowering, and base-class type-ref
construction.

## Watchouts

- `expr/coordinator.cpp` now has no `.tag` reads; `rg -n "base_ts\\.tag|\\.tag"
  src/codegen/lir/hir_to_lir/expr/coordinator.cpp` returns no matches.
- Keep the next packet narrow. The remaining `hir_to_lir.cpp` same-wave
  failures are numerous enough to migrate one boundary at a time.
- The accepted focused baseline remains in `test_before.log`; `test_after.log`
  now contains the successful post-migration proof, not the failed deletion
  probe artifact.

## Proof

Proof command:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_lir_.*|cpp_hir_(sema_canonical_symbol|sema_consteval_type_utils).*structured_metadata|cpp_positive_sema_(c_style_cast_.*field_access|inherited_base_member_access_runtime|inherited_base_aggregate_init_runtime|record_nested_aggregate_member_parse|operator_struct_byval_param|struct_method|template_struct.*)_cpp|positive_sema_ok_call_variadic_aggregate_runtime_c|abi_abi_variadic_struct_result_c|llvm_gcc_c_torture_src_(pta_field_[12]|struct_(aliasing_1|cpy_1|ini_[1-4]|ret_2)|zero_struct_[12])_c|eastl_cpp_external_utility_frontend_basic_cpp)$"' > test_after.log 2>&1`
passed. `test_after.log` reports 38/38 tests passed.
