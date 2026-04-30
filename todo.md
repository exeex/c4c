# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.2
Current Step Title: Remove Parser Declarator And Known-Function Rendered Recovery

## Just Finished

Step 2.2 refined the local using-alias qualified value parsing regression fix
without restoring string-only visible-value APIs. Expression parsing now enters
the typedef cast/constructor branch for qualified names only when the full name
resolves as a type or when structured member-typedef metadata/probing shows the
final segment is a member typedef. `Alias::make()` stays on the value expression
path with the spelled `Alias::make` callee, while
`base_type::allocator_type(7)` remains a qualified member typedef functional
cast.

## Suggested Next

Supervisor can review and commit this Step 2.2 regression slice, then continue
with the next parser-owned rendered recovery cleanup that has a structured key
carrier available without touching HIR/LIR/backend files.

## Watchouts

The const-int blocker from commit `28c1e5c5` remains parked: deleting the
rendered-name `eval_const_int(...)` compatibility overload requires HIR
`NttpBindings` metadata migration. Parser-owned callers should stay on
`TextId` or structured maps, but do not pull `src/frontend/hir` carrier
migration into this parser packet; route that through
`ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md` or a narrower HIR
metadata idea if the supervisor switches scope.

No remaining `resolve_visible_value(std::string_view)` or
`resolve_visible_value_name(const std::string&)` parser/test APIs or callers
were found after this regression fix; production parser call sites remain on
the `TextId` structured overload.

The clang caller query was attempted as requested, but the current
`build/compile_commands.json` did not load `src/frontend/parser/impl/core.cpp`;
the small API surface was verified with `rg` instead.

## Proof

Passed:
`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|cpp_parse_local_using_alias_statement_probe_dump|cpp_positive_sema_qualified_member_typedef_functional_cast_frontend_cpp|cpp_(positive_parser|positive_sema|negative_tests))' --output-on-failure) > test_after.log 2>&1`

`test_after.log` records 928/928 selected tests passed, including
`cpp_parse_local_using_alias_statement_probe_dump`,
`cpp_positive_sema_qualified_member_typedef_functional_cast_frontend_cpp`, and
`frontend_parser_tests`.
