Status: Active
Source Idea Path: ideas/open/84_parser_qualified_name_structured_lookup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Reduce compatibility rendering to bridge-only support on touched paths

# Current Packet

## Just Finished
Completed another plan step 4 bridge cleanup on the parser type-identity path by replacing the remaining template-owner fallback in `parser_types_base.cpp` with `bridge_name_in_context(...)`. Alias-member template-primary lookup no longer rebuilds the owner through `compatibility_namespace_name_in_context(...)` before probing the template registry on this touched path.

## Suggested Next
Re-scan the remaining declaration/import helpers that still call `compatibility_namespace_name_in_context(...)` and decide whether step 4 should take one more narrow bridge-only cleanup packet there or roll forward to step 5 proof closure.

## Watchouts
`has_var_type(...)` and other value-side fallbacks still rely on legacy tables, so the structured-first bridge cleanup should stay on parser type identity paths only. `compatibility_namespace_name_in_context(...)` remains the compatibility fallback inside `bridge_name_in_context(...)`; this slice only removed its direct use as the primary owner render on the touched template-member type path.

## Proof
Passed. Proof run:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|cpp_positive_sema_namespace_struct_type_basic_cpp|cpp_positive_sema_record_member_enum_parse_cpp)$'`
Log: `test_after.log`
