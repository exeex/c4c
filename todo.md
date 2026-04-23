Status: Active
Source Idea Path: ideas/open/84_parser_qualified_name_structured_lookup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Reduce compatibility rendering to bridge-only support on touched paths

# Current Packet

## Just Finished
Updated `lookup_type_in_context(...)` so a structured `QualifiedNameKey` hit now prefers `render_qualified_name(...)` and only falls back to compatibility rendering if the structured render cannot be produced. The touched structured type lookup path no longer treats compatibility rendering as authoritative on the hit branch.

## Suggested Next
Check whether any adjacent structured type lookup helper still prefers compatibility-rendered names on a structured hit, and only then decide whether the remaining bridge fallback can be narrowed further.

## Watchouts
`has_var_type(...)` still uses the legacy value tables, so the structured-first probe must stay narrow and should not turn into a repo-wide var-identity migration. The current slice also depends on the shared qualified-name renderer being available through parser headers, so future cleanup should keep that dependency local.

## Proof
Passed. Proof run:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`
`ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_using_global_scope_decl_parse_cpp$'`
Log: `test_after.log`
