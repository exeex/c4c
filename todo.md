Status: Active
Source Idea Path: ideas/open/165_hir_lowerer_function_context_textid_authority.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Convert Remaining Metadata-Capable Paths

# Current Packet

## Just Finished

Completed plan Step 5 static/global bridge conversion for
`FunctionCtx::static_globals`.

Changed paths:
- `src/frontend/hir/impl/lowerer.hpp`
- `src/frontend/hir/hir_types.cpp`
- `src/frontend/hir/impl/expr/scalar_control.cpp`
- `src/frontend/hir/impl/stmt/decl.cpp`
- `src/frontend/hir/impl/stmt/stmt.cpp`
- `tests/frontend/frontend_hir_lookup_tests.cpp`

Local extern/static bridge insertions now populate
`static_global_ids_by_text_id` when the declaration carries complete source
TextId metadata. `lower_var_expr`, `infer_generic_ctrl_type`, and
`infer_call_result_type_from_callee` consult the structured static/global bridge
before the rendered `static_globals` map, and complete source TextId misses no
longer silently reopen the rendered map. Rendered lookup remains available for
no-metadata references and for explicit generated/rendered compatibility marks.

Focused `frontend_hir_lookup_tests` coverage now proves source TextId hits beat
stale rendered static/global entries, complete source TextId misses reject the
rendered map across value/type/function-pointer lookup, and explicit rendered
compatibility still works.

## Suggested Next

Supervisor can hand this Step 5 static/global bridge slice to reviewer or
acceptance, or choose the next narrow metadata-capable lookup path if any Step 5
blockers remain.

## Watchouts

- `static_globals` is still retained as the rendered compatibility map and for
  existing inspection/test fixtures; source references with complete TextId
  metadata route through `static_global_ids_by_text_id` first.
- No metadata/generated compatibility is explicit: invalid TextId references
  use the rendered map, while valid TextId references require a structured hit
  unless marked in the rendered static/global compatibility sets.

## Proof

Ran delegated proof: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(cpp_hir_template_struct_body_instantiation|llvm_gcc_c_torture_src_20000112_1_c|frontend_hir_tests|frontend_hir_lookup_tests|positive_sema_ok_local_fn_ptr_decl_c|backend_codegen_route_x86_64_function_pointer_param_direct_arg_observe_semantic_bir|cpp_positive_sema_c_style_cast_template_fn_ptr_param_type_parse_cpp|cpp_positive_sema_operator_this_out_of_class_runtime_cpp|cpp_positive_sema_template_deferred_method_cpp|cpp_positive_sema_template_struct_method_cpp|eastl_cpp_external_piecewise_construct_frontend_basic_cpp|eastl_cpp_external_utility_frontend_basic_cpp)$' > test_after.log`.

Result: passed; `test_after.log` contains 12/12 selected tests passing.
