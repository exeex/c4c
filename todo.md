Status: Active
Source Idea Path: ideas/open/165_hir_lowerer_function_context_textid_authority.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Convert Remaining Metadata-Capable Paths

# Current Packet

## Just Finished

Completed plan Step 5 local const binding conversion for
`FunctionCtx::local_const_bindings`.

Changed paths:
- `src/frontend/hir/impl/lowerer.hpp`
- `src/frontend/hir/hir_types.cpp`
- `src/frontend/hir/impl/expr/call.cpp`
- `src/frontend/hir/impl/expr/scalar_control.cpp`
- `src/frontend/hir/impl/stmt/control_flow.cpp`
- `src/frontend/hir/impl/stmt/decl.cpp`
- `src/frontend/hir/impl/stmt/stmt.cpp`
- `src/frontend/hir/impl/stmt/switch.cpp`
- `src/frontend/hir/impl/templates/value_args.cpp`
- `tests/frontend/frontend_hir_lookup_tests.cpp`

Foldable local constexpr/const integer declarations now retain the rendered
`local_const_bindings` compatibility map while also populating source
`TextId`/structured-key maps when the declaration has complete local source
metadata. `make_lowerer_consteval_env` passes those local const metadata maps
into `ConstEvalEnv`, and HIR local-const evaluation call sites now hand through
the current `FunctionCtx` metadata maps. Block and statement-expression scope
rollback preserves/restores the rendered and source local const maps together.

Focused `frontend_hir_lookup_tests` coverage now proves local const insertion
populates source maps, source TextId/key lookup beats a stale rendered name,
complete source misses fail closed instead of reopening the rendered local const
map, and invalid/no-metadata compatibility still uses the rendered map.

## Suggested Next

Supervisor can hand this Step 5 `local_const_bindings` slice to reviewer or
acceptance, or choose the next narrow metadata-capable lookup path if any Step 5
items remain.

## Watchouts

- `local_const_bindings` remains the rendered no-metadata compatibility map.
  Source references with complete local TextId metadata now fail closed for the
  local-const domain when the source maps miss.
- This packet intentionally did not expand into pack parameters or broader
  consteval rewrites.

## Proof

Ran delegated proof: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(cpp_hir_template_struct_body_instantiation|llvm_gcc_c_torture_src_20000112_1_c|frontend_hir_tests|frontend_hir_lookup_tests|positive_sema_ok_local_fn_ptr_decl_c|backend_codegen_route_x86_64_function_pointer_param_direct_arg_observe_semantic_bir|cpp_positive_sema_c_style_cast_template_fn_ptr_param_type_parse_cpp|cpp_positive_sema_operator_this_out_of_class_runtime_cpp|cpp_positive_sema_template_deferred_method_cpp|cpp_positive_sema_template_struct_method_cpp|eastl_cpp_external_piecewise_construct_frontend_basic_cpp|eastl_cpp_external_utility_frontend_basic_cpp|cpp_hir_sema_consteval_type_utils_structured_metadata|cpp_hir_static_member_base_structured_metadata|cpp_hir_template_value_arg_static_member_trait)$' > test_after.log`.

Result: passed; `test_after.log` contains 15/15 selected tests passing.
