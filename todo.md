# Current Packet

Status: Active
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

Continue Step 2.3 with a parser-owned API contraction packet for
`lookup_value_in_context`, `lookup_type_in_context`, and
`lookup_concept_in_context`.

The packet should remove the `std::string_view name` semantic/fallback
parameter from these APIs and update recursive/caller/test call sites to pass
only structured lookup carriers (`context_id`, `TextId`, and
`VisibleNameResult*`). It should delete the `kInvalidText` plus rendered
fallback recovery path in these helpers unless the executor proves a missing
carrier and records a metadata blocker instead of preserving string lookup.

Proof should include a fresh build plus focused parser tests that cover the
existing same-feature drifted-string cases for value, type, and concept lookup.

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
