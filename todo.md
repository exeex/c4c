Status: Active
Source Idea Path: ideas/open/84_parser_qualified_name_structured_lookup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Retarget `using` / alias / import lookup paths to structured identity

# Current Packet

## Just Finished
Updated `lookup_value_in_context(...)` so namespace-visible value lookup probes the structured qualified key before any compatibility-rendered fallback, keeping the Step 3 value path structured-first without widening `has_var_type(...)` into broader identity cleanup.

## Suggested Next
Check whether any other Step 3 namespace-visible value lookup still reaches for compatibility rendering before a structured key probe, and only then decide whether Step 4 helper cleanup is actually needed.

## Watchouts
`has_var_type(...)` still uses the legacy value tables, so the structured-first probe must stay narrow and should not turn into a repo-wide var-identity migration.

## Proof
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`
`ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_using_global_scope_decl_parse_cpp$'`
Passed. Proof log: `test_after.log`
