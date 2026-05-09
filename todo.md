Status: Active
Source Idea Path: ideas/open/158_sema_validate_string_authority_audit.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Global, Function, Enum, and Consteval Lookup Precedence

# Current Packet

## Just Finished

Step 2: Global, Function, Enum, and Consteval Lookup Precedence repaired the
baseline regression in function and overload lookup. `lookup_function_by_name`,
`lookup_ref_overloads_by_name`, and `lookup_cpp_overloads_by_name` now treat a
valid function structured key or using-value-alias target key as authoritative
for that reference, so qualified structured metadata misses fail closed instead
of falling through stale rendered function names. No-metadata rendered
compatibility remains available when the reference carries no structured key.

## Suggested Next

Return to supervisor review for the repaired Step 2 regression and decide
whether this slice is ready to commit or needs broader route review before
continuing Step 2.

## Watchouts

- Function and overload lookup now fail closed for any valid function metadata
  key on the reference, including qualified keys. Value lookup behavior from the
  earlier unqualified slice was not changed in this repair.
- Do not weaken tests or mark supported paths unsupported.
- Existing parser coverage exercised the regression; no test files were changed.

## Proof

Passed: `(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_parser_lookup_authority_tests|positive_sema_|cpp_positive_sema_)') > test_after.log 2>&1`

Proof log: `test_after.log` reports 921/921 matching parser and positive sema
tests passed.
