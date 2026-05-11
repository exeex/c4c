Status: Active
Source Idea Path: ideas/open/165_hir_lowerer_function_context_textid_authority.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Convert Remaining Metadata-Capable Paths

# Current Packet

## Just Finished

Completed plan Step 3 first source lookup conversion for `param_fn_ptr_sigs`.

Changed paths:
- `src/frontend/hir/impl/lowerer.hpp`
- `src/frontend/hir/hir_functions.cpp`
- `src/frontend/hir/hir_types.cpp`
- `tests/frontend/frontend_hir_lookup_tests.cpp`

`append_explicit_callable_param` now records complete source parameters in `param_indices_by_text_id` and records function-pointer parameter signatures in `param_fn_ptr_sigs_by_index` by parameter index. Generated/no-metadata parameter names continue to populate the rendered `param_fn_ptr_sigs` compatibility map.

`infer_call_result_type_from_callee` now resolves parameter function-pointer signatures by callee `unqualified_text_id -> param index -> FnPtrSig` before considering rendered fallback. Rendered parameter-signature lookup is only used when the callee has no source `TextId`.

Focused coverage added to `frontend_hir_lookup_tests` for indexed/TextId lookup winning over rendered spelling, complete TextId miss rejecting rendered fallback, and explicit no-metadata rendered compatibility.

## Suggested Next

Convert the next narrow lookup group only after supervisor selection; `local_fn_ptr_sigs` by resolved local identity is the closest adjacent target, but it needs local identity lookup plumbing and should stay separate from this completed parameter-signature slice.

## Watchouts

- `param_indices_by_text_id` is function-context scoped; it intentionally does not try to solve same-spelled local shadowing.
- Pack-expanded/generated parameters are not entered into the source `TextId` index because their emitted names are generated route-local handles; they retain rendered compatibility.
- `local_fn_ptr_sigs` and ordinary `params`/`locals` lookups still use their existing rendered maps outside this packet.

## Proof

Ran delegated proof: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|frontend_hir_lookup_tests|positive_sema_ok_local_fn_ptr_decl_c|backend_codegen_route_x86_64_function_pointer_param_direct_arg_observe_semantic_bir|cpp_positive_sema_c_style_cast_template_fn_ptr_param_type_parse_cpp)$' > test_after.log`.

Result: passed; `test_after.log` contains 5/5 tests passing.
