# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.1
Current Step Title: Remove Parser Template And NTTP Default Rendered Mirrors

## Just Finished

Step 2.1 deleted the rendered-name compatibility mirrors for parser NTTP
defaults: the `eval_deferred_nttp_default(QualifiedNameKey, string_view, ...)`
and `cache_nttp_default_expr_tokens(QualifiedNameKey, string_view, ...)`
overloads are gone. Parser call sites that already carry
`QualifiedNameKey` now call the structured-key NTTP default API directly, and
the cache path remains keyed by `ParserTemplateState::NttpDefaultExprKey`.

## Suggested Next

Route through supervisor review of Step 2.1 exhaustion, or continue with the
next parser cleanup packet that removes a remaining string-only compatibility
path without crossing into HIR metadata migration.

## Watchouts

The const-int blocker from commit `28c1e5c5` remains parked: deleting the
rendered-name `eval_const_int(...)` compatibility overload requires HIR
`NttpBindings` metadata migration. Parser-owned callers should stay on
`TextId` or structured maps, but do not pull `src/frontend/hir` carrier
migration into this parser packet; route that through
`ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md` or a narrower HIR
metadata idea if the supervisor switches scope.

The string-only `eval_deferred_nttp_default(std::string, ...)` path still
exists outside this packet. It maps text to a structured key before lookup, but
removing it should be handled as a separate cleanup packet unless the supervisor
explicitly widens parser API ownership.

## Proof

Passed:
`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|cpp_(positive_parser|positive_sema|negative_tests))' --output-on-failure) > test_after.log 2>&1`

Result: build succeeded and 927/927 selected tests passed. Proof log:
`test_after.log`.
