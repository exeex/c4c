# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.3
Current Step Title: Audit Parser Type, Tag, And Member-Typedef Routes

## Just Finished

Step 2.3 deleted the remaining parser type/concept lookup string projection
overloads `lookup_type_in_context(..., std::string*)` and
`lookup_concept_in_context(..., std::string*)`. Production parser lookup stays
on `VisibleNameResult`, and focused `frontend_parser_tests` call sites now keep
structured type/concept results until explicit display assertions through
`visible_name_spelling(...)`.

## Suggested Next

Supervisor can review and commit this focused Step 2.3 parser type/concept
lookup API deletion slice. A next packet should remain outside this slice unless
the supervisor chooses to continue the broader rendered-name cleanup plan.

## Watchouts

The const-int blocker from commit `28c1e5c5` remains parked: deleting the
rendered-name `eval_const_int(...)` compatibility overload requires HIR
`NttpBindings` metadata migration. Parser-owned callers should stay on
`TextId` or structured maps, but do not pull `src/frontend/hir` carrier
migration into this parser packet; route that through
`ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md` or a narrower HIR
metadata idea if the supervisor switches scope.

No remaining parser-owned type/concept `lookup_*_in_context(..., std::string*)`
declaration, definition, or caller was found after this packet. Explicit
`visible_name_spelling(...)` projection remains only in tests and production
paths that need rendered spelling for display/diagnostics/output.

## Proof

Passed:
`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|cpp_parse_local_using_alias_statement_probe_dump|cpp_positive_sema_qualified_member_typedef_functional_cast_frontend_cpp|cpp_(positive_parser|positive_sema|negative_tests))' --output-on-failure) > test_after.log 2>&1`

`test_after.log` records 928/928 selected tests passed, including
`cpp_parse_local_using_alias_statement_probe_dump`,
`cpp_positive_sema_qualified_member_typedef_functional_cast_frontend_cpp`, and
`frontend_parser_tests`.
