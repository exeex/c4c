# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.4.4.5A.3
Current Step Title: Review Alias-Template Carrier Route Before Bridge Deletion

## Just Finished

Step 2.4.4.5A.2 is implemented and reviewer-accepted as on-track structured
metadata progress. Alias-template substitution now tries the
structured `ParserAliasTemplateInfo::member_typedef` carrier before the
rendered/deferred bridge, substituting owner args from alias parameter
`TextId`s, probing the concrete owner/member typedef cache by
`TemplateInstantiationKey`, and otherwise selecting the owner template pattern
through the carrier's `QualifiedNameKey` plus member `TextId`. A focused
`frontend_parser_tests` case proves `Alias<int>` resolves through the carrier
even after the rendered/deferred `TypeSpec` owner/member spelling is drifted.
The review gate is not yet satisfied because focused qualified-alias and
alias-of-alias member-typedef drift/disagreement coverage is still missing.

## Suggested Next

Execute a narrow Step 2.4.4.5A.3 proof packet before any Step 2.4.4.5B bridge
deletion: add focused same-feature drift/disagreement coverage for
qualified-alias and alias-of-alias member-typedef carrier consumption, then
run the supervisor-delegated proof command and produce a fresh canonical
`test_after.log`.

## Watchouts

- The structured consumer intentionally bails out for owner args that still
  mention active template-scope parameters so dependent alias-of-alias cases
  keep using the existing dependent/template bridge until concrete
  instantiation.
- The rendered/deferred `TypeSpec` bridge and alias-name-derived `_t` fallback
  remain reachable only after the structured carrier route fails; Step
  2.4.4.5B still owns bridge deletion.
- Do not start Step 2.4.4.5B until Step 2.4.4.5A.3 has focused
  qualified-alias and alias-of-alias disagreement coverage plus fresh
  canonical `test_after.log`.
- The consumer does not infer carrier identity from `tpl_struct_origin`,
  `deferred_member_type_name`, `TypeSpec::tag`, rendered owner/member spelling,
  `TemplateArgRef::debug_text`, saved RHS token reparsing, or a local
  alias-of-alias parser.

## Proof

`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|cpp_parse_local_using_alias_statement_probe_dump|cpp_positive_sema_qualified_member_typedef_functional_cast_frontend_cpp|cpp_(positive_parser|positive_sema|negative_tests))' --output-on-failure) > test_after.log 2>&1`

Passed: 928/928 tests in the delegated subset for Step 2.4.4.5A.2. The
supervisor rolled that proof forward to `test_before.log`; there is no current
`test_after.log` claim for Step 2.4.4.5A.3 yet.
