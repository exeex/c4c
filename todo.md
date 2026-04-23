Status: Active
Source Idea Path: ideas/open/84_parser_qualified_name_structured_lookup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Retarget `using` / alias / import lookup paths to structured identity

# Current Packet

## Just Finished
Retargeted `using` value-import registration to preserve the resolved visible value name while still storing structured `QualifiedNameKey` identity, and isolated namespace value lookup rendering behind a structured-first helper that keeps global `using ::name` aliases on the existing unqualified value-binding path.

## Suggested Next
Continue Step 3 by trimming the remaining compatibility-only value fallback cases around anonymous-namespace recursion and `has_var_type(...)`, and verify whether any namespace-visible value lookups still depend on `compatibility_namespace_name_in_context(...)` outside the new helper boundary.

## Watchouts
`has_var_type(...)` still relies on the legacy value tables, so compatibility rendering remains a bridge-only fallback on the touched `using` path and should stay narrow rather than widening into a repo-wide var-identity migration.

## Proof
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`
`ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_using_global_scope_decl_parse_cpp$'`
Passed. Proof log: `test_after.log`
