Status: Active
Source Idea Path: ideas/open/84_parser_qualified_name_structured_lookup.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Remove the remaining fallback helpers from the primary qualified-lookup path

# Current Packet

## Just Finished
Ran the close gate for step 5 using the existing canonical before/after logs. The focused parser subset still passes cleanly after the bridge cleanup, but the lifecycle close check did not accept closure.

## Suggested Next
Continue the active plan with the newly added slice for the remaining compatibility-backed lookup surfaces. The source idea is not complete yet because fallback helpers still sit behind the semantic path, so the next execution should target that bridge cleanup rather than closing.

## Watchouts
Legacy string-keyed parser tables still exist behind fallback helpers such as `has_typedef_type(...)`, `concept_names`, and the compatibility renderer itself, so this slice should be treated as bridge-surface cleanup rather than a full removal of compatibility storage. The repo regression-guard checker reported a non-increase failure on the 3-test before/after pair even though both logs were 3/3 passing with no new failures, which blocks close under the current strict gate.

## Proof
Passed. Proof run:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|cpp_positive_sema_namespace_struct_type_basic_cpp|cpp_positive_sema_record_member_enum_parse_cpp)$'`
Log: `test_after.log`
Close gate run:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`
Result: failed, passed count did not strictly increase
