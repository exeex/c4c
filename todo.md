Status: Active
Source Idea Path: ideas/open/151_parser_out_of_class_owner_probe_token_sequence.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Thread Structured Owner Identity To Consumers

# Current Packet

## Just Finished

Completed Step 4 LIR/frontend-crash repair for `llvm_gcc_c_torture_src_pr44164_c` after owner-key canonicalization.

- Stopped structured layout observation from recursively re-entering owner/layout lookup for aggregate field types while serializing legacy field parity.
- Added a const-initializer active-layout guard so a stale or missing nested aggregate owner cannot recursively substitute the currently emitted layout.
- Made LIR type declaration emission include `Module::struct_defs` entries that were missing from `struct_def_order`, sorting the fallback tail deterministically while restoring the `%struct.Y` declaration needed by the nested `X -> Y -> YY -> Z` layout.
- Aligned aggregate field type serialization in type declarations with non-recursive field spelling, preventing `%struct.Y` from becoming self-referential.
- Added member access recovery that retries stale `resolved_owner_tag` misses through structured base-owner lookup and, when needed, a unique `member_symbol_id` layout hit.

## Suggested Next

Next packet: repair the two reviewer-identified Step 4 consumer gaps before broadening further: make AST node owner-key helpers intern spelling carriers into `Module::link_name_texts` on first-use collisions, and gate `lower_non_method_functions_and_globals` rendered method fallback after complete structured metadata just like the attached-method path.

## Watchouts

- Do not treat `TextId` alone as semantic compound owner identity.
- Compound owner meaning still lives in owner segment sequences plus namespace/global qualification.
- Do not weaken out-of-class member, constructor, operator, nested-owner, or same-spelling ambiguity tests.
- Keep unrelated parser `token_spelling(...)` cleanup outside this idea unless it feeds owner identity.
- `qualified_owner` and `qualified_owner_tag` still feed template-owner lookup through rendered parser text IDs; do not delete them until a structured lookup replacement exists.
- HIR `try_parse_qualified_struct_method_name` still splits rendered `Node::name`; this slice keeps it only after structured out-of-class metadata is absent or incomplete, not after complete structured misses.
- `make_ns_qual` still uses `qualifier_segments` strings to populate HIR `segment_text_ids`; this packet did not broaden into that shared helper because it has wider declaration/global/type-definition blast radius.
- The helper can only repair parser-owned ids when the link-name table already has or can intern the spelling carrier; consumers without a reliable spelling carrier still need a separate route.
- The no-module `typespec_aggregate_owner_key(TypeSpec)` path remains a structured parser-id fallback because it has no link-name table for cross-table canonicalization.
- This slice does not canonicalize member typedef `tag_text_id` itself; the touched tests keep member ids canonical and isolate owner-key behavior.
- Keep function/global namespace metadata changes out of the next packet unless the owner-key helper naturally supports them without changing lookup behavior.
- `make_out_of_class_struct_method_lookup_key` lives in `hir_build.cpp`; the initial owned-file list named `hir_lowering_core.cpp`, but that draft is not the active consumer for this regression.
- `pr44164.c` still exposes frontend/HIR metadata drift: `struct Y` can exist in `struct_defs` without being listed in `struct_def_order`, and some member expressions carry stale rendered owner tags. This packet handles those facts in LIR consumers without parser changes.
- The const-init active-layout guard is a recursion safety valve; it should not be used to justify accepting future owner/layout cycles when exact metadata is available.
- The `member_symbol_id` recovery intentionally accepts only a unique layout hit.
- Reviewer follow-up: add first-use collision coverage where the real owner spelling is not pre-interned, plus stale rendered qualified-name coverage for a complete structured miss in non-method lowering.

## Proof

Passed: `(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'cpp_hir_module_decl_lookup_structured_mirror|cpp_positive_sema_inherited_implicit_member_out_of_class_runtime_cpp|cpp_positive_sema_local_direct_init_member_call_runtime_cpp|cpp_positive_sema_namespace_dual_using_runtime_cpp|cpp_positive_sema_namespace_function_call_runtime_cpp|cpp_positive_sema_namespace_global_var_runtime_cpp|cpp_positive_sema_namespace_nested_runtime_cpp|cpp_positive_sema_namespace_nested_using_std_runtime_cpp|cpp_positive_sema_namespace_reopen_runtime_cpp|cpp_positive_sema_namespace_reopen_using_std_runtime_cpp|cpp_positive_sema_namespace_struct_collision_runtime_cpp|cpp_positive_sema_namespace_struct_runtime_cpp|cpp_positive_sema_operator_implicit_member_runtime_cpp|cpp_positive_sema_operator_this_out_of_class_runtime_cpp|cpp_positive_sema_qualified_member_typedef_functional_cast_frontend_cpp|cpp_positive_sema_unqualified_static_member_call_runtime_cpp|cpp_positive_sema_using_namespace_fn_lookup_cpp|llvm_gcc_c_torture_src_pr44164_c' && ctest --test-dir build -j --output-on-failure -R 'frontend_hir|frontend_parser_lookup_authority') > test_after.log 2>&1`

Proof log: `test_after.log`.
