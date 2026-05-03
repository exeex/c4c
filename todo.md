# Current Packet

Status: Active
Source Idea Path: ideas/open/142_codegen_lir_aggregate_type_identity_metadata.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Migrate Call, VaArg, Const Init, And ABI Consumers

## Just Finished

Step 4 - Migrate Call, VaArg, Const Init, And ABI Consumers:
migrated the call/variadic aggregate sizing consumers in `call/args.cpp` and
`call/vaarg.cpp` so their `lookup_structured_layout` calls recover and pass an
explicit `StructNameId` carrier from HIR structured owner metadata
(`record_def` / `tag_text_id`) when available. Final LLVM call and `va_arg`
type spelling still comes from the existing `TypeSpec` formatting path, while
the no-carrier layout observation paths are now labeled
`variadic-aggregate-arg-legacy-compat` and `va_arg-aggregate-legacy-compat`.

## Suggested Next

Next coherent packet: migrate the remaining Step 4 aggregate call signature /
return type-ref consumers in `call/target.cpp`, keeping final emitted spelling
separate from structured LIR identity.

## Watchouts

- The call/variadic sizing helpers intentionally use structured identity only
  for layout lookup; packed argument, byval, HFA, and `va_arg` result spelling
  still follows the existing LLVM text path.
- The owner-key recovery helper is local to this bounded packet. A later wider
  Step 4 packet may choose to centralize it if more call/ABI consumers need the
  same carrier recovery.
- Const-init and ABI classification files remain untouched for their own Step 4
  packets.

## Proof

Passed:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_lir_.*|cpp_hir_(sema_canonical_symbol|sema_consteval_type_utils).*structured_metadata|cpp_positive_sema_(c_style_cast_.*field_access|inherited_base_member_access_runtime|inherited_base_aggregate_init_runtime|record_nested_aggregate_member_parse|operator_struct_byval_param|struct_method|template_struct.*)_cpp|positive_sema_ok_call_variadic_aggregate_runtime_c|abi_abi_variadic_struct_result_c|llvm_gcc_c_torture_src_(pta_field_[12]|struct_(aliasing_1|cpy_1|ini_[1-4]|ret_2)|zero_struct_[12])_c|eastl_cpp_external_utility_frontend_basic_cpp)$"' > test_after.log 2>&1`

Matching baseline and patched proof were captured in `test_before.log` and
`test_after.log`. Regression guard passed with 38/38 before and 38/38 after.
