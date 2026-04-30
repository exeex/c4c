# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.2
Current Step Title: Remove Parser Declarator And Known-Function Rendered Recovery

## Just Finished

Step 2.2 started and completed the parser visible-value string-only API cleanup:
`resolve_visible_value(std::string_view)` and
`resolve_visible_value_name(const std::string&)` were deleted from the parser
header/implementation, and the four parser tests that still called the rendered
compatibility projection now pass explicit `TextId` values into the structured
`resolve_visible_value_name(TextId, std::string_view)` overload.

## Suggested Next

Continue Step 2.2 with the next parser-owned rendered recovery cleanup,
prioritizing any remaining declarator or known-function compatibility path that
has a structured key carrier available without touching HIR/LIR/backend files.

## Watchouts

The const-int blocker from commit `28c1e5c5` remains parked: deleting the
rendered-name `eval_const_int(...)` compatibility overload requires HIR
`NttpBindings` metadata migration. Parser-owned callers should stay on
`TextId` or structured maps, but do not pull `src/frontend/hir` carrier
migration into this parser packet; route that through
`ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md` or a narrower HIR
metadata idea if the supervisor switches scope.

No remaining direct string-only `resolve_visible_value(...)` or
`resolve_visible_value_name(...)` parser/test callers were found after deletion;
production parser call sites already use the `TextId` structured overload.

The clang caller query was attempted as requested, but the current
`build/compile_commands.json` did not load `src/frontend/parser/impl/core.cpp`;
the small API surface was verified with `rg` instead.

## Proof

Passed:
`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|cpp_(positive_parser|positive_sema|negative_tests))' --output-on-failure) > test_after.log 2>&1`

`test_after.log` records 927/927 tests passed.
