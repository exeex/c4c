Status: Active
Source Idea Path: ideas/open/150_nttp_type_binding_domain_key_contract.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate Parser Deferred Evaluation APIs

# Current Packet

## Just Finished

Completed `plan.md` Step 3, "Migrate Parser Deferred Evaluation APIs", for parser deferred NTTP evaluation tests and compatibility wrappers.

- Migrated non-compatibility deferred NTTP parser tests to call the structured `ParserTemplateBindingSet` overloads directly.
- Added explicit legacy string-pair compatibility coverage in `test_parser_deferred_nttp_legacy_string_pair_overloads_are_compatibility`.
- Marked the parser legacy string-pair overload bridge as compatibility-only in the API/implementation comments.

## Suggested Next

Next coherent packet: audit whether any remaining parser template-binding wrappers outside deferred NTTP evaluation still need compatibility-only tests or can be retired in a separate bounded slice.

## Watchouts

- The retained legacy string-pair overload use is intentional compatibility coverage only; ordinary parser tests now pass structured binding sets.
- TextId-only test helper bindings are structured-overload inputs but remain compatibility/non-authoritative entries; tests needing semantic authority must build owner/index metadata locally.
- Empty binding cases use `ParserTemplateBindingSet{}` to avoid exercising the legacy overloads accidentally.
- HIR files and production parser paths already using `ParserTemplateBindingSet` were not touched.

## Proof

Ran the delegated proof exactly:

```bash
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_parser_lookup_authority_tests)$') > test_after.log 2>&1
```

Result: passed. The build completed and both delegated parser test binaries passed. Proof log: `test_after.log`.
