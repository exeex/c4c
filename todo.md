# Current Packet

Status: Active
Source Idea Path: ideas/open/143_typespec_identity_normalization_boundary.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Normalize Parser Cast And Template Owner Identity

## Just Finished

Step 4 fixed the Group D parser disambiguation failures from
`review/139_140_141_failure_attribution_review.md` without adding a rendered-tag
consumer fallback. `parse_type_name()` now preserves deferred-member owner
identity only for parenthesized pointer/ref/function abstract type-ids, while
ordinary declarations still go through `parse_base_type()` typedef expansion.
Sema now accepts cast typedefs carrying explicit deferred-member owner metadata.

The focused guard also verifies that `typename Owner<int>::type (&)(int)` and
`typename H<T>::template Rebind<U>::Type (&)(Arg)` keep deferred member TextIds
through abstract declarator application.

## Suggested Next

Continue the normalization mainline against the remaining attribution-review
families. The close bar for idea 143 is now full green:
`ctest --test-dir build -j 8 --output-on-failure` must pass with zero failures,
not merely show fewer failures than the current baseline.

Latest full run still has 19 failures. The Group D generated c-style-cast
matrix failures are gone; remaining work is EASTL/template parsing timeouts or
parse failures, C/LIR StructNameId mirror issues, and the residual C torture
frontend/backend failures listed in `test_after.log`.

## Watchouts

- Do not delete `TypeSpec::tag` during this runbook.
- Do not add another local rendered-tag fallback in a consumer as progress.
- Preserve stale-rendered-spelling disagreement tests.
- Keep idea 141 parked until normalized identity boundaries are stable.
- Do not close idea 143 while full ctest still has failures.

## Proof

Focused proof passed:
`cmake --build build --target c4cll cpp_hir_parser_declarator_deferred_owner_metadata_test`
and
`ctest --test-dir build -j --output-on-failure -R '^(cpp_hir_parser_declarator_deferred_owner_structured_metadata|cpp_positive_sema_consteval_typespec_member_alias_cpp|cpp_positive_sema_template_variable_member_typedef_normalization_runtime_cpp|cpp_hir_template_member_owner_chain|cpp_hir_template_member_owner_field_and_local|cpp_positive_sema____generated_parser_disambiguation_matrix_compile_positive_owner_dependent_(template_member|typename)__decl_(function_lvalue_ref|function_pointer|function_rvalue_ref|member_function_pointer)__ctx_c_style_cast_target__compile_positive_cpp)$'`.

Broad proof is not yet green:
`ctest --test-dir build -j 8 --output-on-failure > test_after.log 2>&1`
reported 19 failures out of 3023.
