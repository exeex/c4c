Status: Active
Source Idea Path: ideas/open/165_hir_lowerer_function_context_textid_authority.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Convert Remaining Metadata-Capable Paths

# Current Packet

## Just Finished

Completed plan Step 5 repair for stale source-local identity after local
`extern` and local `static` declarations install a `static_globals` bridge.

Changed paths:
- `src/frontend/hir/impl/lowerer.hpp`
- `src/frontend/hir/hir_types.cpp`
- `src/frontend/hir/impl/expr/scalar_control.cpp`
- `src/frontend/hir/impl/expr/call.cpp`
- `src/frontend/hir/impl/expr/object.cpp`
- `src/frontend/hir/impl/expr/operator.cpp`
- `src/frontend/hir/impl/stmt/decl.cpp`
- `src/frontend/hir/impl/stmt/range_for.cpp`
- `src/frontend/hir/impl/stmt/stmt.cpp`
- `tests/frontend/frontend_hir_lookup_tests.cpp`

`lower_local_decl_stmt` now clears stale source-local lookup authority when a
same-name local `extern` resolves to a global or a same-name local `static`
lowers into a static/global bridge. The cleanup removes rendered local/name
compatibility entries, rendered function-pointer signatures, and any
`local_ids_by_text_id` entry for either the shadowed rendered local id or the
bridge declaration TextId.

The dense `local_fn_ptr_sigs_by_id` map has no erase API, so the repair fences
those stale signatures by removing the `TextId -> LocalId` authority that made
them reachable. Focused coverage proves local extern/static bridges beat stale
source-local value/type/signature lookup while the existing source-local
hit/miss/generated fallback behavior remains intact.

## Suggested Next

Supervisor can choose the next narrow FunctionCtx lookup group. Ordinary
`local_const_bindings` and pack parameter rendered lookups remain outside this
packet.

## Watchouts

- `DenseIdMap` still has no erase operation; stale dense local signature/type
  slots are fenced by clearing their reachable identity maps rather than
  deleting the dense slots.
- Ordinary `local_const_bindings` rendered lookups remain outside this packet.

## Proof

Ran delegated proof: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(cpp_hir_template_struct_body_instantiation|llvm_gcc_c_torture_src_20000112_1_c|frontend_hir_tests|frontend_hir_lookup_tests|positive_sema_ok_local_fn_ptr_decl_c|backend_codegen_route_x86_64_function_pointer_param_direct_arg_observe_semantic_bir|cpp_positive_sema_c_style_cast_template_fn_ptr_param_type_parse_cpp|cpp_positive_sema_operator_this_out_of_class_runtime_cpp|cpp_positive_sema_template_deferred_method_cpp|cpp_positive_sema_template_struct_method_cpp|eastl_cpp_external_piecewise_construct_frontend_basic_cpp|eastl_cpp_external_utility_frontend_basic_cpp)$' > test_after.log`.

Result: passed; `test_after.log` contains 12/12 selected tests passing.
