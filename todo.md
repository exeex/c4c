# Current Packet

Status: Complete
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.3
Current Step Title: Remove Remaining Parser Semantic Spelling And Fallback Authority

## Just Finished

Step 2.3 removed the remaining fallback spelling authority from
`lookup_using_value_alias(...)`. The helper now requires callers to provide an
existing alias `TextId`, has no `std::string_view` fallback parameter, and no
longer recovers a missing key with `find_parser_text_id(...)`.

Production and focused parser-test call sites now pass only `context_id`, the
alias `TextId`, and the result pointer. `frontend_parser_tests` still covers
structured alias rejection when the target has no semantic binding, explicit
valid-`TextId` no-key compatibility aliases, and invalid-`TextId` rejection.

## Suggested Next

Supervisor can commit this parser API contraction slice, then continue Step
2.3 with the remaining parser semantic lookup APIs that still accept spelling
parameters if the active route stays in parser-owned scope.

## Watchouts

The const-int blocker from commit `28c1e5c5` remains parked: deleting the
rendered-name `eval_const_int(...)` compatibility overload requires HIR
`NttpBindings` metadata migration. Parser-owned callers should stay on
`TextId` or structured maps, but do not pull `src/frontend/hir` carrier
migration into this parser packet; route that through
`ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md` or a narrower HIR
metadata idea if the supervisor switches scope.

`lookup_using_value_alias(...)` now intentionally rejects `kInvalidText`;
future callers that lack a valid alias `TextId` need carrier repair rather
than a rendered spelling lookup path.

## Proof

Passed:
`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|cpp_parse_local_using_alias_statement_probe_dump|cpp_positive_sema_qualified_member_typedef_functional_cast_frontend_cpp|cpp_(positive_parser|positive_sema|negative_tests))' --output-on-failure) > test_after.log 2>&1`

`test_after.log` contains the fresh passing proof: 100% tests passed, 0 tests
failed out of 928.
