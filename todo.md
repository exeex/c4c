# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.1
Current Step Title: Remove Parser Template And NTTP Default Rendered Mirrors

## Just Finished

Step 2.1 completed for the parser NTTP-default cleanup: the legacy
`eval_deferred_nttp_default(std::string, ...)` declaration and definition were
deleted, and the two direct legacy string-only assertions in
`tests/frontend/frontend_parser_tests.cpp` were converted to preserve
structured-key-only rendered-mirror disagreement coverage without calling the
removed API.

## Suggested Next

Run supervisor-side review/validation for Step 2.1 and decide whether the
remaining const-int rendered-name compatibility blocker should stay parked for
the HIR metadata idea or become the next packet.

## Watchouts

The const-int blocker from commit `28c1e5c5` remains parked: deleting the
rendered-name `eval_const_int(...)` compatibility overload requires HIR
`NttpBindings` metadata migration. Parser-owned callers should stay on
`TextId` or structured maps, but do not pull `src/frontend/hir` carrier
migration into this parser packet; route that through
`ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md` or a narrower HIR
metadata idea if the supervisor switches scope.

No remaining direct `eval_deferred_nttp_default(std::string, ...)` callers were
found after deletion; remaining parser call sites use the structured
`QualifiedNameKey` overload.

## Proof

Passed:
`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|cpp_(positive_parser|positive_sema|negative_tests))' --output-on-failure) > test_after.log 2>&1`

`test_after.log` records 927/927 tests passed.
