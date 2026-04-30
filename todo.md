# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.2
Current Step Title: Remove Parser Declarator And Known-Function Rendered Recovery

## Just Finished

Step 2.2 deleted the remaining parser value-lookup string projection overloads
`lookup_using_value_alias(..., std::string*)` and
`lookup_value_in_context(..., std::string*)`. The production alias bridge in
`resolve_visible_type(...)` now carries `VisibleNameResult` until the explicit
display-name projection point, and focused `frontend_parser_tests` call sites
assert structured value results plus `visible_name_spelling(...)` only where a
compatibility spelling is the behavior under test.

## Suggested Next

Supervisor can review and commit this focused Step 2.2 value lookup API deletion
slice. The remaining parser lookup string projection overloads found nearby are
type/concept bridges and should be handled only by a separately delegated packet.

## Watchouts

The const-int blocker from commit `28c1e5c5` remains parked: deleting the
rendered-name `eval_const_int(...)` compatibility overload requires HIR
`NttpBindings` metadata migration. Parser-owned callers should stay on
`TextId` or structured maps, but do not pull `src/frontend/hir` carrier
migration into this parser packet; route that through
`ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md` or a narrower HIR
metadata idea if the supervisor switches scope.

No remaining value-side `lookup_using_value_alias(..., std::string*)` or
`lookup_value_in_context(..., std::string*)` declaration, definition, or caller
was found after this packet. Production parser value lookup call sites remain on
`VisibleNameResult`, with explicit `visible_name_spelling(...)` projection only
where display text is genuinely needed.

## Proof

Passed:
`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|cpp_parse_local_using_alias_statement_probe_dump|cpp_positive_sema_qualified_member_typedef_functional_cast_frontend_cpp|cpp_(positive_parser|positive_sema|negative_tests))' --output-on-failure) > test_after.log 2>&1`

`test_after.log` records 928/928 selected tests passed, including
`cpp_parse_local_using_alias_statement_probe_dump`,
`cpp_positive_sema_qualified_member_typedef_functional_cast_frontend_cpp`, and
`frontend_parser_tests`.
