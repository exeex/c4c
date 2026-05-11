Status: Active
Source Idea Path: ideas/open/165_hir_lowerer_function_context_textid_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Convert Remaining Metadata-Capable FunctionCtx Source Lookups

# Current Packet

## Just Finished

Completed plan Step 4 narrow source lookup conversion for `local_fn_ptr_sigs`.

Changed paths:
- `src/frontend/hir/impl/lowerer.hpp`
- `src/frontend/hir/impl/stmt/decl.cpp`
- `src/frontend/hir/impl/stmt/stmt.cpp`
- `src/frontend/hir/hir_types.cpp`
- `tests/frontend/frontend_hir_lookup_tests.cpp`

`lower_local_decl_stmt` now records complete source locals in `local_ids_by_text_id` and stores function-pointer local signatures in `local_fn_ptr_sigs_by_id` by resolved `LocalId`. Generated/no-metadata local function-pointer declarations continue to populate the rendered `local_fn_ptr_sigs` compatibility map.

`infer_call_result_type_from_callee` now resolves local function-pointer signatures by callee `unqualified_text_id -> LocalId -> FnPtrSig` before considering rendered fallback. Rendered local-signature lookup is only used when the callee has no source `TextId`.

Local scope save/restore now preserves and restores the new local identity/signature maps alongside the existing rendered local maps. Focused coverage added to `frontend_hir_lookup_tests` for LocalId/TextId lookup winning over rendered spelling, complete TextId miss rejecting rendered fallback, and explicit no-metadata rendered compatibility.

## Suggested Next

Convert the next narrow FunctionCtx source lookup group only after supervisor selection; ordinary `params`/`locals` rendered maps remain outside this packet and should be handled separately from local function-pointer signatures.

## Watchouts

- `local_ids_by_text_id` is function-context scoped and restored with local scopes; `local_types` remains dense ID metadata and is not scope-restored by the pre-existing pattern.
- Complete source local function-pointer signatures no longer populate the rendered `local_fn_ptr_sigs` map; only no-metadata/generated compatibility does.
- Ordinary `params`/`locals` rendered lookups and `static_globals` remain outside this packet.

## Proof

Ran delegated proof: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|frontend_hir_lookup_tests|positive_sema_ok_local_fn_ptr_decl_c|backend_codegen_route_x86_64_function_pointer_param_direct_arg_observe_semantic_bir|cpp_positive_sema_c_style_cast_template_fn_ptr_param_type_parse_cpp)$' > test_after.log`.

Result: passed; `test_after.log` contains 5/5 tests passing.
