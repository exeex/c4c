Status: Active
Source Idea Path: ideas/open/152_hir_lir_rendered_owner_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Retire HIR Method Rendered-Name Recovery

# Current Packet

## Just Finished

Completed Plan Step 2 by retiring rendered `Node::name` split recovery from the targeted HIR out-of-class method paths.

`attach_out_of_class_struct_method_defs` now attaches a function body only through `make_out_of_class_struct_method_lookup_key` plus `struct_methods_by_owner_`. `lower_non_method_functions_and_globals` now classifies out-of-class methods only through the same structured key path. The old `try_parse_qualified_struct_method_name` recovery is no longer a semantic fallback in `hir_build.cpp`.

Added focused coverage for rendered-only legacy data: attachment no longer binds a pending method from `RenderedOnlyOwner::run` without structured owner metadata, and non-method lowering no longer suppresses such a function as an out-of-class method.

## Suggested Next

Next coherent packet: choose the next rendered-owner authority family from the Step 1 inventory, preferably the shared HIR owner-key bridge around `make_ast_node_unqualified_text_id_for_owner_key` / `make_struct_def_node_owner_key` only if the supervisor wants to move from method attachment into aggregate/owner-key canonicalization.

## Watchouts

- `try_parse_qualified_struct_method_name` still exists in `hir_lowering_core.cpp`, but the targeted `hir_build.cpp` call sites no longer use it.
- Rendered `struct_methods_` compatibility maps still exist for other lookup paths outside this packet.
- Keep the next packet separate from parser owner-probe code, aggregate layout compatibility, member typedef recovery, and LIR lowering unless the supervisor explicitly selects one of those families.

## Proof

Supervisor-selected proof passed and wrote `test_after.log`:

`cmake --build --preset default --target c4cll frontend_hir_lookup_tests && ctest --test-dir build -R '^(frontend_hir_lookup_tests|cpp_positive_sema_namespaced_out_of_class_method_context_frontend_cpp|cpp_positive_sema_qualified_namespaced_out_of_class_method_context_frontend_cpp|cpp_positive_sema_out_of_class_member_owner_scope_parse_cpp|cpp_positive_sema_operator_this_out_of_class_runtime_cpp)$' --output-on-failure`
