# Current Packet

Status: Active
Source Idea Path: ideas/open/142_codegen_lir_aggregate_type_identity_metadata.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Probe TypeSpec Tag Removal Boundary

## Just Finished

Step 5 - Probe TypeSpec Tag Removal Boundary:
migrated the two `src/codegen/lir/hir_to_lir/hir_to_lir.cpp` deletion-probe
boundaries off direct `TypeSpec::tag` reads. `object_align_bytes` now takes the
aggregate path based on the semantic base kind and resolves layout through
`lookup_structured_layout`/`structured_layout_align_bytes`; flexible-array
global lowering now resolves the HIR aggregate definition through
`find_typespec_aggregate_layout`.

## Suggested Next

Next coherent packet: migrate the remaining same-wave deletion-probe boundary in
`src/codegen/lir/hir_to_lir/lvalue.cpp`, where member field access still falls
back from structured identity to `access.base_ts.tag`. Prefer existing
structured layout/name helpers over adding a new rendered-tag lookup path.

## Watchouts

- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp` no longer contains `.tag` reads;
  remaining `struct_defs.find(...)` calls in that file use declaration-order or
  base-tag strings, not `TypeSpec::tag`.
- Keep the next packet scoped to `lvalue.cpp` unless a tiny existing shared
  helper naturally clears the boundary.

## Proof

Passed:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_lir_.*|cpp_hir_(sema_canonical_symbol|sema_consteval_type_utils).*structured_metadata|cpp_positive_sema_(c_style_cast_.*field_access|inherited_base_member_access_runtime|inherited_base_aggregate_init_runtime|record_nested_aggregate_member_parse|operator_struct_byval_param|struct_method|template_struct.*)_cpp|positive_sema_ok_call_variadic_aggregate_runtime_c|abi_abi_variadic_struct_result_c|llvm_gcc_c_torture_src_(pta_field_[12]|struct_(aliasing_1|cpy_1|ini_[1-4]|ret_2)|zero_struct_[12])_c|eastl_cpp_external_utility_frontend_basic_cpp)$"' > test_after.log 2>&1`

`test_after.log` now contains the accepted executor proof for this packet:
build passed, and the selected CTest subset passed 38/38.
