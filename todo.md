# Current Packet

Status: Active
Source Idea Path: ideas/open/142_codegen_lir_aggregate_type_identity_metadata.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Migrate Call, VaArg, Const Init, And ABI Consumers

## Just Finished

Step 4 - Migrate Call, VaArg, Const Init, And ABI Consumers:
migrated the remaining ABI aggregate classification/layout helpers in
`calling_convention.cpp` and the narrow LIR-side ABI helpers in `core.cpp` so
AMD64 size/classification, AArch64 HFA classification, and statement object
alignment first recover aggregate definitions through HIR structured owner
metadata (`record_def` / `tag_text_id`). Existing `TypeSpec::tag` map lookups
are retained only as explicit legacy compatibility fallbacks, and final LLVM ABI
spelling remains unchanged.

## Suggested Next

Next coherent packet: run the Step 5 temporary `TypeSpec::tag` deletion probe,
record the first remaining codegen/LIR residual boundary, and revert the probe
edit before handing back.

## Watchouts

- This packet deliberately keeps the HIR `struct_defs` bridge as the source of
  ABI layout facts after resolving the canonical owner key; it does not remove
  legacy field/tag storage.
- The new owner-key helpers are local copies matching accepted call, va_arg,
  lvalue, and const-init packets. Centralizing them would be a separate cleanup,
  not part of the deletion-probe boundary.
- `lookup_structured_layout` still records rendered type names for observation
  text and legacy parity, but `stmt-object-align` now supplies the structured
  `StructNameId` when one is available.

## Proof

Passed:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_lir_.*|cpp_hir_(sema_canonical_symbol|sema_consteval_type_utils).*structured_metadata|cpp_positive_sema_(c_style_cast_.*field_access|inherited_base_member_access_runtime|inherited_base_aggregate_init_runtime|record_nested_aggregate_member_parse|operator_struct_byval_param|struct_method|template_struct.*)_cpp|positive_sema_ok_call_variadic_aggregate_runtime_c|abi_abi_variadic_struct_result_c|llvm_gcc_c_torture_src_(pta_field_[12]|struct_(aliasing_1|cpy_1|ini_[1-4]|ret_2)|zero_struct_[12])_c|eastl_cpp_external_utility_frontend_basic_cpp)$"' > test_after.log 2>&1`

Matching baseline and patched proof were captured in `test_before.log` and
`test_after.log`. Regression guard passed with 38/38 before and 38/38 after.
