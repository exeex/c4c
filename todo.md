# Current Packet

Status: Active
Source Idea Path: ideas/open/142_codegen_lir_aggregate_type_identity_metadata.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Probe TypeSpec Tag Removal Boundary

## Just Finished

Step 5 - Probe TypeSpec Tag Removal Boundary:
migrated the ABI aggregate layout fallback in
`src/codegen/llvm/calling_convention.cpp` off direct `TypeSpec::tag` reads.
`lookup_struct` now delegates to the shared aggregate layout helper, which uses
structured owner metadata first and compatibility spelling fallback second. The
shared helper now preserves the old ABI behavior by trying the final aggregate
spelling if the rendered compatibility tag does not name a `mod.struct_defs`
entry.

## Suggested Next

Next coherent packet: rerun the temporary `TypeSpec::tag` deletion probe and
move to the next boundary. The expected remaining boundary is
`src/codegen/lir/verify.cpp:664`, where direct aggregate signature verification
still gates on `TypeSpec::tag`.

## Watchouts

- The probe edit was temporary: `const char* tag` in
  `src/frontend/parser/ast.hpp` was restored before the post-probe rebuild.
- `types.cpp` still writes `FieldStep::tag` for GEP text and observes
  `tag_text_id`; those are not `TypeSpec::tag` residuals.
- `src/codegen/shared/llvm_helpers.hpp` still contains the intentionally named
  compatibility helpers that read or write legacy tags when the field exists.
- Keep verifier checks as the next packet unless the deletion probe exposes a
  different first compile boundary.

## Proof

Proof command:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_lir_.*|cpp_hir_(sema_canonical_symbol|sema_consteval_type_utils).*structured_metadata|cpp_positive_sema_(c_style_cast_.*field_access|inherited_base_member_access_runtime|inherited_base_aggregate_init_runtime|record_nested_aggregate_member_parse|operator_struct_byval_param|struct_method|template_struct.*)_cpp|positive_sema_ok_call_variadic_aggregate_runtime_c|abi_abi_variadic_struct_result_c|llvm_gcc_c_torture_src_(pta_field_[12]|struct_(aliasing_1|cpy_1|ini_[1-4]|ret_2)|zero_struct_[12])_c|eastl_cpp_external_utility_frontend_basic_cpp)$"' > test_after.log 2>&1`
passed. `test_after.log` is the current proof log.
