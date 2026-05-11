Status: Active
Source Idea Path: ideas/open/165_hir_lowerer_function_context_textid_authority.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Convert Remaining Metadata-Capable Paths

# Current Packet

## Just Finished

Completed plan Step 5 final reviewer blocker repair for parameter
function-pointer return inference.

Changed paths:
- `todo.md`
- `src/frontend/hir/hir_types.cpp`
- `tests/frontend/frontend_hir_lookup_tests.cpp`

`Lowerer::infer_call_result_type_from_callee` now treats a complete source
parameter `TextId` hit as authoritative for function-pointer signature lookup.
If `param_indices_by_text_id` resolves the parameter but
`param_fn_ptr_sigs_by_index` has no signature for that index, inference returns
`std::nullopt` immediately instead of falling through to rendered parameter,
static/global bridge, module global, or module function lookup.

The static/global bridge check now runs after the parameter authority block, so
parameter identity shadows static/global fallback consistently with value/type
lookup.

Added focused coverage proving a same-spelled rendered parameter signature,
static/global bridge, module global function-pointer signature, and module
function return type are all rejected after the complete parameter `TextId` hit
has no indexed function-pointer signature.

## Suggested Next

Supervisor can review and commit this blocker repair, then continue with plan
Step 6 final proof and closure readiness.

## Watchouts

- The change intentionally preserves rendered parameter fn-ptr compatibility
  for no-metadata callees and explicit rendered compatibility paths.
- `test_after.log` was overwritten by the delegated proof command as required.

## Proof

Ran delegated proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(cpp_hir_template_struct_body_instantiation|llvm_gcc_c_torture_src_20000112_1_c|frontend_hir_tests|frontend_hir_lookup_tests|positive_sema_ok_local_fn_ptr_decl_c|backend_codegen_route_x86_64_function_pointer_param_direct_arg_observe_semantic_bir|cpp_positive_sema_c_style_cast_template_fn_ptr_param_type_parse_cpp|cpp_positive_sema_operator_this_out_of_class_runtime_cpp|cpp_positive_sema_template_deferred_method_cpp|cpp_positive_sema_template_struct_method_cpp|eastl_cpp_external_piecewise_construct_frontend_basic_cpp|eastl_cpp_external_utility_frontend_basic_cpp|cpp_hir_sema_consteval_type_utils_structured_metadata|cpp_hir_static_member_base_structured_metadata|cpp_hir_template_value_arg_static_member_trait)$' > test_after.log`

Result: passed; 15/15 selected tests passed. Proof log: `test_after.log`.
