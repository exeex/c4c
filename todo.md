Status: Active
Source Idea Path: ideas/open/84_parser_qualified_name_structured_lookup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Retarget `using` / alias / import lookup paths to structured identity

# Current Packet

## Just Finished
Added a structured qualified-name helper for the `using` importer, switched value-alias import recording to store `QualifiedNameKey` identity before any compatibility string bridge, and tightened parser tests so alias visibility still resolves when the bridge spelling is corrupted.

## Suggested Next
Continue Step 3 by isolating the remaining string-keyed namespace value fallback inside `lookup_value_in_context(...)`, especially the anonymous-namespace and `has_var_type(...)` compatibility cases that still require rendered bridge names.

## Watchouts
`has_var_type(...)` still relies on the legacy value tables, so compatibility rendering remains a bridge-only fallback on the touched `using` path and should stay narrow rather than widening into a repo-wide var-identity migration.

## Proof
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`
Passed. Proof log: `test_after.log`
