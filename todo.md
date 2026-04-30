# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.3
Current Step Title: Remove Remaining Parser Semantic Spelling And Fallback Authority

## Just Finished

Step 2.3 removed the remaining parser semantic spelling/fallback parameters
from `lookup_value_in_context(...)`, `lookup_type_in_context(...)`, and
`lookup_concept_in_context(...)`. The helpers now accept only `context_id`, the
structured `TextId`, and `VisibleNameResult*`, and they reject `kInvalidText`
instead of recovering lookup authority from rendered spellings.

Production recursive and qualified/visible lookup call sites were updated to
use the contracted APIs. Focused parser tests now cover the existing drifted
string cases for value, type, and concept lookup while asserting invalid
`TextId` rejection for the formerly preserved rendered fallback routes.

## Suggested Next

Supervisor should review the Step 2.3 parser API contraction diff and decide
whether this completes the active packet or needs a plan-owner/reviewer pass
before commit.

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

The three contracted parser lookup helpers now follow the same rule: callers
without a valid `TextId` must repair metadata before invoking the helper.

## Proof

Passed:
`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|cpp_parse_local_using_alias_statement_probe_dump|cpp_positive_sema_qualified_member_typedef_functional_cast_frontend_cpp|cpp_(positive_parser|positive_sema|negative_tests))' --output-on-failure) > test_after.log 2>&1`

`test_after.log` contains the fresh passing proof for this packet: 100% tests
passed, 0 tests failed out of 928.
