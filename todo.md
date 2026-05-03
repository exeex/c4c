# Current Packet

Status: Active
Source Idea Path: ideas/open/142_codegen_lir_aggregate_type_identity_metadata.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Probe TypeSpec Tag Removal Boundary

## Just Finished

Step 5 - Probe TypeSpec Tag Removal Boundary:
migrated the shared codegen helper boundary exposed by the deletion probe.
`llvm_ty`, `llvm_alloca_ty`, and `llvm_field_ty` now obtain final LLVM
aggregate spelling through a dedicated final-spelling helper instead of
directly reading `TypeSpec::tag`; `sizeof_ts` now prefers structured
`HirRecordOwnerKey` layout lookup and uses rendered compatibility only through
an explicit fallback helper. The exposed call and `va_arg` residual checks now
use structured aggregate presence or explicit legacy-tag compatibility helpers
instead of direct `tag` reads.

## Suggested Next

Next coherent packet: rerun the temporary `TypeSpec::tag` deletion probe and
classify the new first compile boundary. If the first boundary is still within
codegen aggregate identity, migrate that one family; if it is outside this
idea, split it into a follow-up idea instead of widening this runbook.

## Watchouts

- Final LLVM type spelling is intentionally preserved. Helpers without a
  `Module` still use available `TypeSpec` spelling metadata for output text,
  while helpers with a `Module` can use `tag_text_id` and structured owner
  indexes before explicit compatibility fallback.
- `typespec_legacy_tag_if_present` is an explicit compatibility bridge and is
  SFINAE-shaped so a future deletion probe can compile past these sites.
- The next deletion probe may expose broader codegen residuals outside the
  owned helper/call/va_arg boundary; do not fold unrelated ABI, const-init,
  lvalue, or type-chain routes into this packet retroactively.

## Proof

Proof command:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_lir_.*|cpp_hir_(sema_canonical_symbol|sema_consteval_type_utils).*structured_metadata|cpp_positive_sema_(c_style_cast_.*field_access|inherited_base_member_access_runtime|inherited_base_aggregate_init_runtime|record_nested_aggregate_member_parse|operator_struct_byval_param|struct_method|template_struct.*)_cpp|positive_sema_ok_call_variadic_aggregate_runtime_c|abi_abi_variadic_struct_result_c|llvm_gcc_c_torture_src_(pta_field_[12]|struct_(aliasing_1|cpy_1|ini_[1-4]|ret_2)|zero_struct_[12])_c|eastl_cpp_external_utility_frontend_basic_cpp)$"' > test_after.log 2>&1`

Matching baseline and patched proof were captured in `test_before.log` and
`test_after.log`. Regression guard passed with 38/38 before and 38/38 after.
