# Current Packet

Status: Active
Source Idea Path: ideas/open/142_codegen_lir_aggregate_type_identity_metadata.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Migrate Call, VaArg, Const Init, And ABI Consumers

## Just Finished

Step 4 - Migrate Call, VaArg, Const Init, And ABI Consumers:
migrated the central const-init aggregate layout/definition bridge in
`const_init_emitter.cpp` so `lookup_const_init_struct_def` first recovers a
`StructNameId` carrier from HIR structured owner metadata (`record_def` /
`tag_text_id`) and passes it to `lookup_structured_layout` at the
`const-init-aggregate` site. Carrierless aggregate types are now labeled through
the explicit `const-init-aggregate-legacy-compat` observation path, and final
constant/GEP LLVM spelling remains on the existing `TypeSpec` formatting path.

## Suggested Next

Next coherent packet: migrate the remaining Step 4 ABI classification consumers
off semantic `TypeSpec::tag` authority when structured metadata is available,
without changing rendered LLVM ABI spelling.

## Watchouts

- Const-init field walking still consumes the legacy `HirStructDef`; this
  packet only routes that lookup through structured layout metadata. If a
  structured layout has no legacy `HirStructDef` equivalent, const-init still
  returns no definition rather than fabricating one.
- The owner-key recovery helper is local to this bounded packet. A later wider
  Step 4 packet may choose to centralize it if ABI consumers need the same
  carrier recovery.
- ABI classification files remain untouched for their own Step 4 packet.

## Proof

Passed:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_lir_.*|cpp_hir_(sema_canonical_symbol|sema_consteval_type_utils).*structured_metadata|cpp_positive_sema_(c_style_cast_.*field_access|inherited_base_member_access_runtime|inherited_base_aggregate_init_runtime|record_nested_aggregate_member_parse|operator_struct_byval_param|struct_method|template_struct.*)_cpp|positive_sema_ok_call_variadic_aggregate_runtime_c|abi_abi_variadic_struct_result_c|llvm_gcc_c_torture_src_(pta_field_[12]|struct_(aliasing_1|cpy_1|ini_[1-4]|ret_2)|zero_struct_[12])_c|eastl_cpp_external_utility_frontend_basic_cpp)$"' > test_after.log 2>&1`

Matching baseline and patched proof were captured in `test_before.log` and
`test_after.log`. Regression guard passed with 38/38 before and 38/38 after.
