Status: Active
Source Idea Path: ideas/open/84_parser_qualified_name_structured_lookup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Reduce compatibility rendering to bridge-only support on touched paths

# Current Packet

## Just Finished
Completed another plan step 4 bridge cleanup on declaration-side parser lookup by replacing the remaining direct `compatibility_namespace_name_in_context(...)` registrations in `parser_declarations.cpp` with `bridge_name_in_context(...)`. Concept names, namespace-scoped `using` aliases/imports, and template-struct qualified registration on this touched path now go through the structured-first bridge instead of rebuilding canonical owner strings directly.

## Suggested Next
Decide whether step 4 is now complete and move to step 5 proof closure, or take one final narrow packet only if a remaining direct semantic hot-path call outside `parser_declarations.cpp` can be demoted to bridge-only support without widening into legacy value-table redesign.

## Watchouts
`has_var_type(...)`, `lookup_type_in_context(...)`, and other parser-core fallback helpers still rely on legacy string-keyed tables, so the structured-first bridge cleanup should stay on touched registration/lookup paths only. `compatibility_namespace_name_in_context(...)` remains the fallback implementation behind `bridge_name_in_context(...)`; this slice only removed its direct use as the primary declaration-side render path.

## Proof
Passed. Proof run:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|cpp_positive_sema_namespace_struct_type_basic_cpp|cpp_positive_sema_record_member_enum_parse_cpp)$'`
Log: `test_after.log`
