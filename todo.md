# Current Packet

Status: Active
Source Idea Path: ideas/open/142_codegen_lir_aggregate_type_identity_metadata.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Establish Aggregate Identity Carrier Contract and Migrate Layout And Field Access Consumers

## Just Finished

Step 2/3 - Establish Aggregate Identity Carrier Contract and Migrate Layout And Field Access Consumers:
extended `stmt_emitter_detail::lookup_structured_layout` so callers can supply
`StructNameId` directly and only fall back to the rendered `TypeSpec::tag`
lookup as explicitly named legacy compatibility. Migrated the `types.cpp`
field-chain family so recursive anonymous-member/base traversal recovers child
aggregate identity from structured LIR field mirrors and carries that
`structured_name_id` into `FieldStep`; union child recursion remains on the
legacy recovery path because union slot 0 is not a per-field identity carrier.

## Suggested Next

Next coherent packet: migrate one more `lookup_structured_layout` consumer that
already has a nearby structured carrier, with `lvalue.cpp` indexed aggregate GEP
or call/variadic aggregate sizing as the likely next boundary. Keep any
`mod.struct_defs.find(tag)` path explicitly labeled as legacy compatibility.

## Watchouts

- The `types.cpp` field-chain cluster moved for structured child aggregate
  identity, but entry into the family still starts from a legacy tag because
  `resolve_member_field_access` has no direct owner `StructNameId` carrier yet.
- A temporary `TypeSpec::tag` deletion probe was not run for this packet: the
  current known first failure remains broader final-spelling and legacy-lookup
  surfaces such as `llvm_helpers.hpp:444`, so the packet proof is build/test
  only rather than a useful deletion probe.
- Do not pass structured child identity through union field recursion unless a
  per-field union carrier is added; the current structured union layout has a
  single payload slot.

## Proof

Passed:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_lir_.*|cpp_hir_(sema_canonical_symbol|sema_consteval_type_utils).*structured_metadata|cpp_positive_sema_(c_style_cast_.*field_access|inherited_base_member_access_runtime|inherited_base_aggregate_init_runtime|record_nested_aggregate_member_parse|operator_struct_byval_param|struct_method|template_struct.*)_cpp|positive_sema_ok_call_variadic_aggregate_runtime_c|abi_abi_variadic_struct_result_c|llvm_gcc_c_torture_src_(pta_field_[12]|struct_(aliasing_1|cpy_1|ini_[1-4]|ret_2)|zero_struct_[12])_c|eastl_cpp_external_utility_frontend_basic_cpp)$"' > test_after.log 2>&1`

Matching clean baseline and patched proof were captured in `test_before.log`
and `test_after.log`. Regression guard passed with 38/38 before and 38/38 after.
The broader candidate also exposed pre-existing clean-HEAD failure
`llvm_gcc_c_torture_src_struct_ret_1_c`; it was excluded from the accepted
focused subset.
