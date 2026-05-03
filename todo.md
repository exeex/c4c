# Current Packet

Status: Active
Source Idea Path: ideas/open/142_codegen_lir_aggregate_type_identity_metadata.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Probe TypeSpec Tag Removal Boundary

## Just Finished

Step 5 - Probe TypeSpec Tag Removal Boundary:
migrated the front-of-file aggregate type-ref helper family in
`src/codegen/lir/hir_to_lir/hir_to_lir.cpp`. `lir_owned_type_spec` now routes
legacy tag ownership through the shared compatibility setter, and
`lir_field_type_ref`, `lir_global_type_ref`, and `lir_signature_type_ref` now
resolve aggregate `StructNameId` mirrors from structured owner metadata or
explicit compatibility helpers instead of reading `TypeSpec::tag` directly.
The nearby base-class type-ref construction was adjusted mechanically to pass
the already-rendered struct name id into the shared aggregate-ref constructor.

## Suggested Next

Next coherent packet: rerun a temporary `TypeSpec::tag` deletion probe and use
the first remaining compile boundary to pick the next narrow migration. The
expected next boundaries are object alignment and flexible-array global
lowering in `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`, plus the same-wave
`lvalue.cpp` member-access residual if the probe reaches it.

## Watchouts

- `lir_owned_type_spec` still preserves the existing verifier contract by
  calling `set_typespec_legacy_tag_if_present`; the direct `.tag` write moved
  behind an explicitly named shared compatibility helper.
- Aggregate `StructNameId` candidates are accepted only when their rendered LLVM
  struct name matches the mirrored text, avoiding owner-key drift such as
  `%struct.left` shadowing `%struct.Pair`.
- Remaining `hir_to_lir.cpp` direct `.tag` users are outside this helper packet:
  module object alignment and flexible-array global lowering.

## Proof

Proof command:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_lir_.*|cpp_hir_(sema_canonical_symbol|sema_consteval_type_utils).*structured_metadata|cpp_positive_sema_(c_style_cast_.*field_access|inherited_base_member_access_runtime|inherited_base_aggregate_init_runtime|record_nested_aggregate_member_parse|operator_struct_byval_param|struct_method|template_struct.*)_cpp|positive_sema_ok_call_variadic_aggregate_runtime_c|abi_abi_variadic_struct_result_c|llvm_gcc_c_torture_src_(pta_field_[12]|struct_(aliasing_1|cpy_1|ini_[1-4]|ret_2)|zero_struct_[12])_c|eastl_cpp_external_utility_frontend_basic_cpp)$"' > test_after.log 2>&1`
passed, with 38/38 tests passing. Proof log: `test_after.log`.

Baseline review:
rejected `test_baseline.new.log` for commit `fa4aaedc7`: the accepted
baseline is 3000/3000, while the candidate had 129 failures out of 3023.
The failed candidate log was deleted; keep `test_baseline.log` unchanged.
