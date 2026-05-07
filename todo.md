Status: Active
Source Idea Path: ideas/open/146_qualified_name_deferred_carrier_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Route Eager Sema Qualified Lookup Through Domain Tables

# Current Packet

## Just Finished

Step 4: Route Eager Sema Qualified Lookup Through Domain Tables tightened Sema `TypeSpec` name identity so `record_def` metadata is authoritative when present, complete namespace/tag/qualifier TextId identity is required before text-key comparison, and partial qualified metadata blocks rendered-spelling fallback instead of comparing as a stale collision.

## Suggested Next

Supervisor review and commit this Step 4 Sema eager lookup identity slice, then choose the next bounded qualified-name authority packet.

## Watchouts

`src/frontend/parser/impl/support.cpp` has a separate parser-support nominal compatibility helper with its own rendered fallback behavior; this packet only tightened the delegated Sema helper in `src/frontend/sema/type_utils.cpp`. Keep future slices on structured record/typedef metadata rather than same-spelling rendered tags.

## Proof

`cd /workspaces/c4c && { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_hir_sema_consteval_type_utils_structured_metadata|cpp_positive_sema_qualified_namespaced_out_of_class_method_context_frontend_cpp|cpp_positive_sema_namespace_using_decl_typedef_and_value_frontend_cpp)$'; } > test_after.log 2>&1`

Proof passed. `test_after.log` is the canonical executor proof log: all four delegated tests passed, including focused stale/collision `TypeSpec` equality coverage in `frontend_parser_lookup_authority_tests`.
