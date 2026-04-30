# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.2
Current Step Title: Remove Parser Declarator And Known-Function Rendered Recovery

## Just Finished

Step 2.2 deleted the parser qualified-value string projection API
`resolve_qualified_value_name(const QualifiedNameRef&)` after `rg` found only a
white-box test caller. The compatibility check in `frontend_parser_tests` now
asserts the structured `resolve_qualified_value(...)` result and projects
display text with `visible_name_spelling(...)` only where the test checks the
TextId-less compatibility spelling.

## Suggested Next

Supervisor can review and commit this focused Step 2.2 API deletion slice, then
continue with the next parser-owned rendered recovery cleanup that already has
a structured key carrier available without touching HIR/LIR/backend files.

## Watchouts

The const-int blocker from commit `28c1e5c5` remains parked: deleting the
rendered-name `eval_const_int(...)` compatibility overload requires HIR
`NttpBindings` metadata migration. Parser-owned callers should stay on
`TextId` or structured maps, but do not pull `src/frontend/hir` carrier
migration into this parser packet; route that through
`ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md` or a narrower HIR
metadata idea if the supervisor switches scope.

No remaining `resolve_qualified_value_name` declaration, definition, or caller
was found after this packet. Production parser call sites remain on
`resolve_qualified_value(...)` and explicit `visible_name_spelling(...)`
projection where display text is genuinely needed.

## Proof

Passed:
`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|cpp_parse_local_using_alias_statement_probe_dump|cpp_positive_sema_qualified_member_typedef_functional_cast_frontend_cpp|cpp_(positive_parser|positive_sema|negative_tests))' --output-on-failure) > test_after.log 2>&1`

`test_after.log` records 928/928 selected tests passed, including
`cpp_parse_local_using_alias_statement_probe_dump`,
`cpp_positive_sema_qualified_member_typedef_functional_cast_frontend_cpp`, and
`frontend_parser_tests`.
