Status: Active
Source Idea Path: ideas/open/146_qualified_name_deferred_carrier_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Route Eager Sema Qualified Lookup Through Domain Tables

# Current Packet

## Just Finished

Step 4: Route Eager Sema Qualified Lookup Through Domain Tables closed the consteval value/NTTP lookup escape hatch that reopened rendered NTTP names after structured key or TextId domain metadata authoritatively missed, and added a qualified stale-rendered collision test for that value-domain behavior.

## Suggested Next

Supervisor can review and commit this consteval lookup-authority slice, then choose the next Step 4 value-domain lookup packet.

## Watchouts

Rendered `ConstMap` compatibility still remains for true no-metadata environments; the new guard only blocks fallback after a populated structured/TextId lookup domain has missed.

## Proof

`cd /workspaces/c4c && { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_hir_sema_consteval_type_utils_structured_metadata|cpp_positive_sema_cpp20_if_constexpr_concept_id_frontend_cpp|cpp_positive_sema_template_constexpr_member_runtime_cpp|cpp_positive_sema_template_type_traits_builtin_cpp)$'; } > test_after.log 2>&1`

Proof passed. `test_after.log` is the canonical executor proof log: `frontend_parser_lookup_authority_tests`, `cpp_hir_sema_consteval_type_utils_structured_metadata`, `cpp_positive_sema_cpp20_if_constexpr_concept_id_frontend_cpp`, `cpp_positive_sema_template_constexpr_member_runtime_cpp`, and `cpp_positive_sema_template_type_traits_builtin_cpp` passed.
