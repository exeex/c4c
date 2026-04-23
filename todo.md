Status: Active
Source Idea Path: ideas/open/84_parser_qualified_name_structured_lookup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Retarget `using` / alias / import lookup paths to structured identity

# Current Packet

## Just Finished
Updated the `using` value-import registration path so it tries the structured `QualifiedNameKey` rendering first, then falls back to the compatibility-resolved name only if needed. The imported value alias now keeps structured identity primary without widening the Step 3 change into broader value-table cleanup.

## Suggested Next
Check whether any other Step 3 value-import or alias registration path still prefers compatibility-rendered names before a structured key probe, and only then decide whether Step 4 helper cleanup is actually needed.

## Watchouts
`has_var_type(...)` still uses the legacy value tables, so the structured-first probe must stay narrow and should not turn into a repo-wide var-identity migration. The current slice also depends on the shared qualified-name renderer being available through parser headers, so future cleanup should keep that dependency local.

## Proof
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`
`ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_using_global_scope_decl_parse_cpp$'`
Passed. Proof log: `test_after.log`
