Status: Active
Source Idea Path: ideas/open/165_hir_lowerer_function_context_textid_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Convert Remaining Metadata-Capable FunctionCtx Source Lookups

# Current Packet

## Just Finished

Completed plan Step 4 regression fix for source parameter lookup versus
rendered compatibility parameter lookup.

Changed paths:
- `src/frontend/hir/impl/lowerer.hpp`
- `src/frontend/hir/hir_functions.cpp`
- `src/frontend/hir/hir_types.cpp`
- `src/frontend/hir/impl/expr/scalar_control.cpp`
- `src/frontend/hir/impl/stmt/stmt.cpp`
- `tests/frontend/frontend_hir_lookup_tests.cpp`

`lower_var_expr` now resolves ordinary parameter values by
`param_indices_by_text_id` when the reference carries a complete source
`unqualified_text_id`. A complete source parameter TextId miss no longer
reopens the rendered `ctx.params` compatibility map.

`infer_generic_ctrl_type` now resolves ordinary parameter types through the
same `TextId -> param index -> fn.params[index]` path, and function-pointer
parameter return inference uses the same source-first rule. Rendered
`ctx.params` fallback remains for no-metadata references and for generated or
synthetic parameters explicitly marked in `FunctionCtx` as rendered
compatibility by TextId/name.

Implicit `this` insertion and generated/non-source callable parameters now mark
their rendered compatibility identity without adding source
`param_indices_by_text_id` authority. Focused coverage in
`frontend_hir_lookup_tests` includes the valid-TextId generated `this` style
compatibility case plus the prior source-hit/source-miss/no-metadata cases.

## Suggested Next

Supervisor can choose the next narrow FunctionCtx lookup group. Ordinary
`locals`, `static_globals`, `local_const_bindings`, and pack parameter rendered
lookups remain outside this packet.

## Watchouts

- A valid `n->unqualified_text_id` alone is not enough to identify a source
  parameter reference. Source authority is still `param_indices_by_text_id`;
  rendered fallback with a valid TextId requires the new explicit
  `FunctionCtx` compatibility marker.
- Generated `this` can carry a TextId that does not match the HIR rendered name
  TextId, so the compatibility marker tracks both generated TextIds and
  generated rendered names.
- Ordinary `locals`, `static_globals`, and `local_const_bindings` rendered
  lookups remain outside this packet.

## Proof

Ran delegated proof: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(cpp_hir_template_struct_body_instantiation|llvm_gcc_c_torture_src_20000112_1_c|frontend_hir_tests|frontend_hir_lookup_tests|positive_sema_ok_local_fn_ptr_decl_c|backend_codegen_route_x86_64_function_pointer_param_direct_arg_observe_semantic_bir|cpp_positive_sema_c_style_cast_template_fn_ptr_param_type_parse_cpp)$' > test_after.log`.

Then ran delegated follow-up proof: `ctest --test-dir build -j --output-on-failure -R '^(cpp_positive_sema_operator_this_out_of_class_runtime_cpp|cpp_positive_sema_template_deferred_method_cpp|cpp_positive_sema_template_struct_method_cpp|eastl_cpp_external_piecewise_construct_frontend_basic_cpp|eastl_cpp_external_utility_frontend_basic_cpp)$' >> test_after.log`.

Result: passed; `test_after.log` contains 7/7 tests passing for the first
subset and 5/5 tests passing for the follow-up subset.
