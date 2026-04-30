# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.1
Current Step Title: Remove Parser Template And NTTP Default Rendered Mirrors

## Just Finished

Step 2.1 removed the now-unused rendered
`make_template_struct_instance_key(...)` helper from parser type helpers. No
rendered template-instantiation de-dup helper remains reachable under that
name; parser de-dup authority stays on
`ParserTemplateState::TemplateInstantiationKey` /
`instantiated_template_struct_keys_by_key`.

## Suggested Next

Continue with the supervisor-selected next Step 2 parser cleanup packet, or
route through plan review if the supervisor considers Step 2.1 exhausted.

## Watchouts

The const-int blocker from commit `28c1e5c5` remains parked: deleting the
rendered-name `eval_const_int(...)` compatibility overload requires HIR
`NttpBindings` metadata migration. Parser-owned callers should stay on
`TextId` or structured maps, but do not pull `src/frontend/hir` carrier
migration into this parser packet; route that through
`ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md` or a narrower HIR
metadata idea if the supervisor switches scope.

The legacy string overloads for `eval_deferred_nttp_default(...)` and
`cache_nttp_default_expr_tokens(...)` still exist for callers outside this owned
packet, but they now delegate to structured-key lookup and ignore the rendered
name as semantic authority.

## Proof

Passed:
`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|cpp_(positive_parser|positive_sema|negative_tests))' --output-on-failure) > test_after.log 2>&1`

Result: build succeeded and 927/927 tests passed. Proof log:
`test_after.log`.
