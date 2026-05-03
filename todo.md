# Current Packet

Status: Active
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

## Suggested Next

Next coherent packet: rerun the temporary `TypeSpec::tag` deletion probe and
classify the next boundary. If the remaining failures are only
`tests/frontend/frontend_parser_tests.cpp`, keep them out of this codegen/LIR
packet family and route them back to the parent frontend parser-test
classification.

## Watchouts

- `src/codegen/lir/verify.cpp` no longer contains direct `.tag` reads; the
  verifier route now uses the named aggregate helper plus the existing
  `StructNameId` declaration lookup.
- `types.cpp` still writes `FieldStep::tag` for GEP text and observes
  `tag_text_id`; those are not `TypeSpec::tag` residuals.
- `src/codegen/llvm/calling_convention.cpp` no longer contains `.tag` reads;
  do not reopen that boundary unless a fresh deletion probe reports a new
  codegen-owned residual.
- Do not modify frontend parser tests as part of the verifier packet.

## Proof

Proof command:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_lir_.*|cpp_hir_(sema_canonical_symbol|sema_consteval_type_utils).*structured_metadata|cpp_positive_sema_(c_style_cast_.*field_access|inherited_base_member_access_runtime|inherited_base_aggregate_init_runtime|record_nested_aggregate_member_parse|operator_struct_byval_param|struct_method|template_struct.*)_cpp|positive_sema_ok_call_variadic_aggregate_runtime_c|abi_abi_variadic_struct_result_c|llvm_gcc_c_torture_src_(pta_field_[12]|struct_(aliasing_1|cpy_1|ini_[1-4]|ret_2)|zero_struct_[12])_c|eastl_cpp_external_utility_frontend_basic_cpp)$"' > test_after.log 2>&1`
passed. `test_after.log` contains the successful build and 38/38 focused tests
passing.
