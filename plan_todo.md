# Positive Template/Sema Regression Todo

Status: Active
Source Idea: ideas/open/template_positive_sema_regressions_plan.md
Source Plan: plan.md

## Active Item

- Step 5: record the completed reference-returning call lvalue slice and queue
  the next named positive template/sema failure

## In Progress

- None

## Completed

- Selected `ideas/open/template_positive_sema_regressions_plan.md` as the
  active source idea
- Created `plan.md` and `plan_todo.md` for execution
- Captured the full-suite baseline in `test_before.log`
- Classified the first patch target as a `PARSE_FAIL` in
  `cpp_positive_sema_eastl_probe_pack_expansion_template_arg_parse_cpp`
- Added focused parser coverage in
  `template_pack_expansion_template_arg_expr_parse.cpp`
- Fixed expression-side template-id parsing to accept trailing `...` after a
  successfully parsed template argument list
- Verified targeted parser cases pass:
  `eastl_probe_pack_expansion_template_arg_parse`,
  `template_pack_expansion_template_arg_expr_parse`,
  `template_template_param_parse`, and `template_variadic_qualified_parse`
- Captured a clean full-suite validation in `test_after.log`
- Verified monotonic full-suite results:
  `test_before.log` = 7 failing / 2236 total,
  `test_after.log` = 6 failing / 2237 total
- Classified `cpp_positive_sema_eastl_probe_call_result_lvalue_frontend_cpp`
  as a reference-returning call lvalue/codegen failure at
  `StmtEmitter: cannot take lval of expr`
- Added focused frontend coverage in
  `call_expr_ref_return_lvalue_frontend.cpp`
- Fixed `StmtEmitter` lvalue emission so call expressions returning references
  reuse the call's storage pointer as an addressable lvalue
- Verified targeted frontend cases pass:
  `call_expr_ref_return_lvalue_frontend` and
  `eastl_probe_call_result_lvalue_frontend`
- Captured a clean scratch rebuild/full-suite validation in `test_after.log`
- Verified monotonic full-suite results:
  `test_before.log` = 7 failing / 2237 total,
  `test_after.log` = 5 failing / 2238 total
- Observed collateral improvement:
  `llvm_gcc_c_torture_src_pr28982b_c` no longer fails after the call-result
  lvalue fix

## Next Slice

- Characterize `cpp_positive_sema_eastl_type_traits_signed_helper_base_expr_parse_cpp`
  and isolate the dependent base-expression parse/runtime mechanism behind it

## Blockers

- None recorded

## Resume Notes

- Start from the named six positive template/sema tests in `plan.md`
- First slice is the parser mechanism for `Types...` inside nested template
  argument lists such as `__and_<is_default_constructible<Types>..., ...>`
- The pack-expansion parse failure is fixed; remaining named failures are the
  dependent base-expression parse/runtime case, template arg deduction, mixed
  params, and template type substitution
- This iteration is focused only on the reference-returning call assignment
  mechanism used by `capacity_ptr() = g_begin + n;`
- `eastl_probe_initializer_list_runtime_cpp` still fails in the suite but it is
  not one of the six named plan-goal cases; do not switch to it unless it
  blocks a planned slice
