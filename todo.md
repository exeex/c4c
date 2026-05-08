Status: Active
Source Idea Path: ideas/open/152_hir_lir_rendered_owner_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory HIR LIR Compatibility Authority

# Current Packet

## Just Finished

Completed Plan Step 1 inventory for rendered-owner compatibility authority in the delegated HIR/LIR scope.

Classified inventory:

- `src/frontend/hir/hir_build.cpp`, `src/frontend/hir/hir_lowering_core.cpp`: `attach_out_of_class_struct_method_defs` and `lower_non_method_functions_and_globals` prefer `make_out_of_class_struct_method_lookup_key` but still fall back to `try_parse_qualified_struct_method_name`, which splits rendered `Node::name` on `::` and looks up `struct_methods_`. Classification: semantic authority fallback. Source idea owner: HIR lowerer. Route note: already blocked when complete structured metadata exists, so the first packet can focus on removing/demoting only this rendered-name fallback after verifying structured registration coverage.
- `src/frontend/hir/hir_build.cpp`, `src/frontend/hir/hir_types.cpp`, `src/codegen/shared/llvm_helpers.hpp`: `make_ast_node_unqualified_text_id_for_owner_key`, `make_struct_def_node_owner_key`, `typespec_record_def_owner_key`, and the cross-table `typespec_aggregate_owner_key(ts, mod)` intern rendered names into `link_name_texts` to repair parser/module `TextId` mismatch. Classification: semantic authority bridge when direct structured owner lookup misses. Source idea owner: HIR owner-key bridge. Route note: this is broader than the method packet because aggregate, typedef, and LIR callers share it.
- `src/codegen/shared/llvm_helpers.hpp`, `src/frontend/hir/hir_types.cpp`, `src/frontend/hir/impl/stmt/decl.cpp`, `src/frontend/hir/impl/expr/builtin.cpp`: `typespec_aggregate_compatibility_tag`, `typespec_aggregate_final_spelling`, `find_typespec_aggregate_layout`, `find_struct_def_by_layout_compatibility_tag`, `rendered_typespec_tag_for_compatibility`, and declaration aggregate compatibility recover layout or owner tags from rendered strings after structured lookup misses. Classification: semantic authority compatibility fallback. Source idea owner: HIR/LIR aggregate layout bridge.
- `src/frontend/hir/hir_functions.cpp`, `src/frontend/hir/impl/templates/member_typedef.cpp`, `src/frontend/hir/impl/templates/type_resolution.cpp`, `src/frontend/hir/impl/templates/value_args.cpp`: `complete_signature_type_member_lookup_key`, `member_typedef_compatibility_name`, `resolve_struct_member_typedef_type(owner_name, ...)`, `legacy_owner_tag_from_type_if_no_metadata`, and related member-type callers use deferred owner keys first but still recover owner/member identity from legacy rendered tags or qualified spelling when metadata is incomplete. Classification: semantic authority fallback. Source idea owner: HIR member typedef resolver.
- `src/frontend/hir/hir_types.cpp`, `src/frontend/hir/impl/expr/scalar_control.cpp`, `src/frontend/hir/impl/templates/value_args.cpp`: member/static lookup helpers build owner/member lookup keys first, but rendered maps such as `struct_static_member_decls_`, `struct_static_member_const_values_`, `member_symbols`, and generated-member payload extraction from rendered names remain secondary semantic lookup routes. Classification: mixed semantic fallback and compatibility/parity instrumentation. Source idea owner: HIR member typedef resolver where it recovers member type/value identity, otherwise HIR lowerer for method/static-member call routing.
- `src/codegen/lir/hir_to_lir/core.cpp`, `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`, `src/codegen/lir/hir_to_lir/lvalue.cpp`, `src/codegen/lir/hir_to_lir/call/args.cpp`, `src/codegen/lir/hir_to_lir/call/target.cpp`, `src/codegen/lir/hir_to_lir/const_init_emitter.cpp`, `src/codegen/lir/hir_to_lir/types.cpp`: `lookup_structured_layout_compatibility_decl`, `lookup_structured_name_id_by_owner_or_compatibility`, `lir_owned_type_spec`, `lir_aggregate_structured_name_id`, `member_access_owner_tag_from_type`, and call/GEP helpers use structured owner keys first but fall back to compatibility tags, final spelling, or rendered LLVM struct names. Classification: semantic authority fallback plus observation/parity reporting. Source idea owner: LIR lowering, with shared HIR/LIR aggregate layout bridge dependency.
- `src/codegen/shared/llvm_helpers.hpp`: `llvm_ty`, `llvm_alloca_ty`, `llvm_field_ty`, `llvm_struct_type_str`, `sanitize_llvm_ident`, and LLVM identifier quoting render final aggregate names for IR text. Classification: display/emission spelling, not owner authority when used only to print an already-selected type.
- `src/codegen/lir/hir_to_lir/core.cpp`: `record_structured_layout_observation` records legacy/structured parity information. Classification: diagnostic/observation, allowed if it does not choose semantic ownership.

## Suggested Next

First implementation packet: retire the rendered `Node::name` split fallback for out-of-class method attachment/classification when structured method-owner metadata is absent.

Owned implementation files for the packet:

- `src/frontend/hir/hir_build.cpp`
- focused tests only if needed under the existing frontend/HIR out-of-class method coverage

Packet shape:

- Verify `make_out_of_class_struct_method_lookup_key` has complete owner and method `TextId` coverage for the focused namespaced/out-of-class method cases.
- In `attach_out_of_class_struct_method_defs` and `lower_non_method_functions_and_globals`, remove or strictly demote the `try_parse_qualified_struct_method_name` fallback so rendered `Node::name` splitting is no longer semantic authority for HIR method attachment.
- Do not touch parser owner-probe code, aggregate layout code, member typedef code, or LIR lowering in this packet.

Supervisor-selected proof command for that packet:

`cmake --build --preset default --target c4cll frontend_hir_lookup_tests && ctest --test-dir build -R '^(frontend_hir_lookup_tests|cpp_positive_sema_namespaced_out_of_class_method_context_frontend_cpp|cpp_positive_sema_qualified_namespaced_out_of_class_method_context_frontend_cpp|cpp_positive_sema_out_of_class_member_owner_scope_parse_cpp|cpp_positive_sema_operator_this_out_of_class_runtime_cpp)$' --output-on-failure`

## Watchouts

- Do not reopen parser owner-probe classification completed by idea 151.
- Do not treat a single rendered `TextId` containing `A::B` as semantic compound-owner identity.
- Do not claim progress by renaming rendered-owner fallback while preserving it as semantic authority.
- Do not weaken tests, mark cases unsupported, or rewrite expectations only to match changed rendered names.
- Keep broad backend rewrites outside this idea unless they are required for focused aggregate owner/layout recovery.
- The HIR method packet should remain separate from `make_ast_node_unqualified_text_id_for_owner_key` and aggregate `typespec_aggregate_owner_key(ts, mod)` repair; those routes are shared and need their own proof.
- LIR aggregate compatibility is not a good first packet because `lookup_structured_layout` and call/GEP/ABI helpers share the same fallback surface.

## Proof

Inventory-only packet; no build proof required.

Local validation for this packet: `git diff --check`.
