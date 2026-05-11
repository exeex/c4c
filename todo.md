Status: Active
Source Idea Path: ideas/open/165_hir_lowerer_function_context_textid_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Convert Remaining Metadata-Capable FunctionCtx Source Lookups

# Current Packet

## Just Finished

Completed plan Step 4 narrow source lookup conversion for ordinary source
parameter value/type lookup.

Changed paths:
- `src/frontend/hir/hir_types.cpp`
- `src/frontend/hir/impl/expr/scalar_control.cpp`
- `tests/frontend/frontend_hir_lookup_tests.cpp`

`lower_var_expr` now resolves ordinary parameter values by
`param_indices_by_text_id` when the reference carries a complete source
`unqualified_text_id`. A complete source parameter TextId miss no longer
reopens the rendered `ctx.params` compatibility map.

`infer_generic_ctrl_type` now resolves ordinary parameter types through the
same `TextId -> param index -> fn.params[index]` path. Rendered `ctx.params`
fallback remains only for no-metadata/synthetic references with
`kInvalidText`, such as compatibility references to `this` and generated pack
elements.

Focused coverage added to `frontend_hir_lookup_tests` for parameter TextId
lookup winning over rendered spelling in both value lowering and generic type
inference, complete source TextId miss rejecting rendered fallback, and
explicit no-metadata rendered compatibility.

## Suggested Next

Convert the next narrow FunctionCtx source lookup group only after supervisor
selection; ordinary `locals`, `static_globals`, `local_const_bindings`, and
pack parameter rendered lookups remain outside this packet.

## Watchouts

- This packet intentionally uses `n->unqualified_text_id` as the source
  metadata signal. `make_unqualified_text_id` may intern rendered names for
  no-metadata nodes, so it is not used to decide whether rendered `ctx.params`
  fallback is allowed.
- The existing rendered `ctx.params["this"]` call sites in statement/call
  lowering remain as synthetic compatibility paths.
- Ordinary `locals`, `static_globals`, and `local_const_bindings` rendered
  lookups remain outside this packet.

## Proof

Ran delegated proof: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|frontend_hir_lookup_tests|positive_sema_ok_local_fn_ptr_decl_c|backend_codegen_route_x86_64_function_pointer_param_direct_arg_observe_semantic_bir|cpp_positive_sema_c_style_cast_template_fn_ptr_param_type_parse_cpp)$' > test_after.log`.

Result: passed; `test_after.log` contains 5/5 tests passing.
