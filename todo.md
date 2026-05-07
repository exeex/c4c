Status: Active
Source Idea Path: ideas/open/146_qualified_name_deferred_carrier_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Route Eager Sema Qualified Lookup Through Domain Tables

# Current Packet

## Just Finished

Step 4: Route Eager Sema Qualified Lookup Through Domain Tables repaired the direct HIR consteval value/NTTP rendered compatibility blocker by distinguishing NTTP-domain metadata misses from unrelated value-domain misses, kept qualified structured value misses authoritative, and preserved the qualified consteval function miss test from the prior slice.

## Suggested Next

Supervisor can review and commit this Step 4 consteval lookup-authority slice, then choose the next qualified lookup authority packet.

## Watchouts

Rendered NTTP compatibility is retained only for unqualified HIR-owned no-context lookups when NTTP TextId/structured metadata has not missed. Qualified value-domain structured misses still block rendered NTTP fallback, and consteval function rendered fallback remains blocked after populated structured/text function-domain misses.

## Proof

`cd /workspaces/c4c && { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|frontend_parser_tests|cpp_hir_sema_consteval_type_utils_structured_metadata|cpp_positive_sema_cpp20_if_constexpr_concept_id_frontend_cpp|cpp_positive_sema_template_constexpr_member_runtime_cpp|cpp_positive_sema_template_type_traits_builtin_cpp)$'; } > test_after.log 2>&1`

Proof passed. `test_after.log` is the canonical executor proof log: `frontend_parser_lookup_authority_tests`, `frontend_parser_tests`, `cpp_hir_sema_consteval_type_utils_structured_metadata`, `cpp_positive_sema_cpp20_if_constexpr_concept_id_frontend_cpp`, `cpp_positive_sema_template_constexpr_member_runtime_cpp`, and `cpp_positive_sema_template_type_traits_builtin_cpp` passed.
