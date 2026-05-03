# Current Packet

Status: Complete
Source Idea Path: ideas/open/142_codegen_lir_aggregate_type_identity_metadata.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Probe TypeSpec Tag Removal Boundary

## Just Finished

Step 5 - Probe TypeSpec Tag Removal Boundary:
migrated the LIR verifier direct aggregate signature boundary in
`src/codegen/lir/verify.cpp` off the local `TypeSpec::tag` gate.
`expected_direct_aggregate_signature_id` now classifies direct named aggregates
through `llvm_helpers::is_named_aggregate_value(type)` before resolving the
rendered LLVM type to a declared `StructNameId`, keeping any legacy tag
compatibility behind the shared helper boundary.

The follow-up temporary deletion probe commented out `TypeSpec::tag` in
`src/frontend/parser/ast.hpp` and ran `cmake --build --preset default >
test_after.log 2>&1`. The probe compile reached codegen/LIR objects without a
codegen hot-region failure and first failed in frontend/HIR test fixtures:
`tests/frontend/frontend_parser_tests.cpp`,
`tests/frontend/frontend_parser_lookup_authority_tests.cpp`,
`tests/frontend/cpp_hir_template_type_arg_encoding_test.cpp`,
`tests/frontend/cpp_hir_static_member_base_metadata_test.cpp`, and
`tests/frontend/frontend_hir_lookup_tests.cpp`. The probe edit was restored,
and a normal `cmake --build --preset default` passed.

## Suggested Next

Next lifecycle action: ask the plan owner to decide whether this codegen/LIR
follow-up idea should close or deactivate. The remaining `TypeSpec::tag`
deletion-probe boundary is frontend/HIR test-fixture cleanup rather than a
codegen/LIR aggregate identity residual, so do not continue it in this packet
family.

## Watchouts

- `src/codegen/lir/verify.cpp` no longer contains direct `.tag` reads; the
  verifier route now uses the named aggregate helper plus the existing
  `StructNameId` declaration lookup.
- `types.cpp` still writes `FieldStep::tag` for GEP text and observes
  `tag_text_id`; those are not `TypeSpec::tag` residuals.
- `src/codegen/llvm/calling_convention.cpp` no longer contains `.tag` reads;
  do not reopen that boundary unless a fresh deletion probe reports a new
  codegen-owned residual.
- Do not modify frontend/HIR tests as part of this codegen/LIR packet family.

## Proof

Proof command:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_lir_.*|cpp_hir_(sema_canonical_symbol|sema_consteval_type_utils).*structured_metadata|cpp_positive_sema_(c_style_cast_.*field_access|inherited_base_member_access_runtime|inherited_base_aggregate_init_runtime|record_nested_aggregate_member_parse|operator_struct_byval_param|struct_method|template_struct.*)_cpp|positive_sema_ok_call_variadic_aggregate_runtime_c|abi_abi_variadic_struct_result_c|llvm_gcc_c_torture_src_(pta_field_[12]|struct_(aliasing_1|cpy_1|ini_[1-4]|ret_2)|zero_struct_[12])_c|eastl_cpp_external_utility_frontend_basic_cpp)$"' > test_after.log 2>&1`
passed. `test_after.log` contains the successful build and 38/38 focused tests
passing.

Deletion probe:
`cmake --build --preset default > test_after.log 2>&1` failed as expected after
temporarily removing `TypeSpec::tag`; first residuals were frontend/HIR test
fixtures, not codegen/LIR hot regions. Restored build:
`cmake --build --preset default` passed.
