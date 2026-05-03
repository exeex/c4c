# Current Packet

Status: Active
Source Idea Path: ideas/open/142_codegen_lir_aggregate_type_identity_metadata.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate Layout And Field Access Consumers

## Just Finished

Step 3 - Migrate Layout And Field Access Consumers:
migrated the `lvalue.cpp` indexed aggregate GEP route so
`StmtEmitter::indexed_gep_elem_ty` / `emit_indexed_gep` accept a narrow
`StructNameId` carrier and recover that carrier from HIR structured owner
metadata (`record_def` / `tag_text_id`) when available. The emitted LLVM type
spelling still comes from the existing `TypeSpec` formatting path, while the
rendered-tag recovery path is now labeled
`indexed-gep-aggregate-legacy-compat`.

## Suggested Next

Next coherent packet: migrate call/variadic aggregate sizing consumers that
already route through `lookup_structured_layout`, keeping final call/ABI
spelling separate from any structured aggregate identity carrier.

## Watchouts

- `indexed_gep_elem_ty` still derives final LLVM text from `llvm_ty` /
  `llvm_alloca_ty`; the new carrier is only used for structured LIR identity.
- Pointer arithmetic call sites outside the direct `IndexExpr` path rely on
  the helper's internal structured-owner recovery unless a future packet passes
  a more local carrier.
- The field-chain entry path still starts from a legacy owner tag unless a real
  structured carrier is nearby.

## Proof

Passed:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_lir_.*|cpp_hir_(sema_canonical_symbol|sema_consteval_type_utils).*structured_metadata|cpp_positive_sema_(c_style_cast_.*field_access|inherited_base_member_access_runtime|inherited_base_aggregate_init_runtime|record_nested_aggregate_member_parse|operator_struct_byval_param|struct_method|template_struct.*)_cpp|positive_sema_ok_call_variadic_aggregate_runtime_c|abi_abi_variadic_struct_result_c|llvm_gcc_c_torture_src_(pta_field_[12]|struct_(aliasing_1|cpy_1|ini_[1-4]|ret_2)|zero_struct_[12])_c|eastl_cpp_external_utility_frontend_basic_cpp)$"' > test_after.log 2>&1`

Matching baseline and patched proof were captured in `test_before.log` and
`test_after.log`. Regression guard passed with 38/38 before and 38/38 after.
