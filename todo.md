# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.3
Current Step Title: Remove Remaining Parser Semantic Spelling And Fallback Authority

## Just Finished

Step 2.3 removed the value-alias to visible-type rendered-spelling semantic
re-entry in `resolve_visible_type(...)`. The alias branch now keeps
`lookup_using_value_alias(...)` results as structured `QualifiedNameKey` and
target `TextId` metadata, then checks structured typedef, alias-template,
local typedef, or direct target `TextId` typedef state without projecting
through `visible_name_spelling(...)` or recovering a `TextId` from rendered
output.

`frontend_parser_tests` now makes a stale rendered `ns::Target` typedef
disagree with the structured namespace typedef target, proving the structured
alias metadata wins over the rendered-spelling fallback.

## Suggested Next

Continue Step 2.3 with the remaining parser semantic lookup APIs that still
accept spelling/fallback parameters, starting with the smallest route where
`lookup_value_in_context`, `lookup_type_in_context`, or
`lookup_concept_in_context` can consume existing structured carriers without
changing source intent.

## Watchouts

The const-int blocker from commit `28c1e5c5` remains parked: deleting the
rendered-name `eval_const_int(...)` compatibility overload requires HIR
`NttpBindings` metadata migration. Parser-owned callers should stay on
`TextId` or structured maps, but do not pull `src/frontend/hir` carrier
migration into this parser packet; route that through
`ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md` or a narrower HIR
metadata idea if the supervisor switches scope.

The alias-template metadata check added to the value-alias/type branch is
structured-key based; do not replace it with a rendered-name probe when
continuing fallback removal.

## Proof

Passed:
`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|cpp_parse_local_using_alias_statement_probe_dump|cpp_positive_sema_qualified_member_typedef_functional_cast_frontend_cpp|cpp_(positive_parser|positive_sema|negative_tests))' --output-on-failure) > test_after.log 2>&1`

`test_after.log` contains the fresh passing proof: 100% tests passed, 0 tests
failed out of 928.
