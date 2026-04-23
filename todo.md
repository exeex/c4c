Status: Active
Source Idea Path: ideas/open/84_parser_qualified_name_structured_lookup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Reduce compatibility rendering to bridge-only support on touched paths

# Current Packet

## Just Finished
Completed plan step 4 on the record/enum tag canonicalization seam by making `bridge_name_in_context(...)` structured-first and routing touched parser type-tag paths through it. Forward declarations, record definitions, and enum tags in namespace context now prefer structured rendering before the compatibility bridge.

## Suggested Next
Check the remaining owner/type helper in `parser_types_base.cpp` that still rebuilds `canonical_owner` with `compatibility_namespace_name_in_context(...)` while searching template struct primaries, and only then decide whether another narrow structured-first bridge cleanup is warranted.

## Watchouts
`has_var_type(...)` and other value-side fallbacks still rely on legacy tables, so the structured-first bridge cleanup should stay on parser type identity paths only. `compatibility_namespace_name_in_context(...)` is still the required bridge fallback when token text or path rendering is unavailable; this slice only demoted it from the primary path on touched record/enum tag flows.

## Proof
Passed. Proof run:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|cpp_positive_sema_namespace_struct_type_basic_cpp|cpp_positive_sema_record_member_enum_parse_cpp)$'`
Log: `test_after.log`
