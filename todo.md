# Current Packet

Status: Active
Source Idea Path: ideas/open/142_codegen_lir_aggregate_type_identity_metadata.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Probe TypeSpec Tag Removal Boundary

## Just Finished

Step 5 - Probe TypeSpec Tag Removal Boundary:
migrated the `src/codegen/lir/hir_to_lir/core.cpp` first-boundary family off
direct semantic `TypeSpec::tag` reads. Structured layout observations now name
the observed type through the resolved `StructNameId`; `lookup_structured_layout`
now prefers HIR owner metadata and uses a named compatibility lookup only after
owner lookup misses; `lookup_abi_struct_layout` now shares that compatibility
route instead of reading `ts.tag` directly.

## Suggested Next

Next coherent packet: rerun the temporary deletion probe and migrate the next
reported boundary, expected to be `src/codegen/lir/hir_to_lir/expr/coordinator.cpp`
member function-pointer signature discovery before widening into
`src/codegen/lir/hir_to_lir/hir_to_lir.cpp` aggregate type-ref,
flexible-array, or base-class helper routes.

## Watchouts

- `typespec_aggregate_compatibility_tag` can produce a metadata spelling that
  is not present in `mod.struct_defs` for legacy C const-init cases; the
  `core.cpp` compatibility lookup now falls back through
  `typespec_aggregate_final_spelling` before returning no layout.
- `core.cpp` still reads structured metadata such as `tag_text_id`; the direct
  semantic `TypeSpec::tag` reads in the delegated family are gone.
- Keep parser/Sema/HIR files out of the next packet unless the deletion probe
  reveals that metadata is genuinely missing rather than merely not consumed.

## Proof

Passed. Ran the supervisor-selected proof command:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_lir_.*|cpp_hir_(sema_canonical_symbol|sema_consteval_type_utils).*structured_metadata|cpp_positive_sema_(c_style_cast_.*field_access|inherited_base_member_access_runtime|inherited_base_aggregate_init_runtime|record_nested_aggregate_member_parse|operator_struct_byval_param|struct_method|template_struct.*)_cpp|positive_sema_ok_call_variadic_aggregate_runtime_c|abi_abi_variadic_struct_result_c|llvm_gcc_c_torture_src_(pta_field_[12]|struct_(aliasing_1|cpy_1|ini_[1-4]|ret_2)|zero_struct_[12])_c|eastl_cpp_external_utility_frontend_basic_cpp)$"' > test_after.log 2>&1`

`test_after.log` now contains the passing build and focused subset result:
38/38 tests passed.
