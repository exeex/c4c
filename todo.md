# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.4.4.5B
Current Step Title: Replace The Dependent/Template Member-Typedef Bridge

## Just Finished

Step 2.4.4.5B deleted the alias-template member-typedef fallback block that
derived owner/member identity from rendered `TypeSpec` spelling,
alias-name-derived `_t` recovery, and local rendered/deferred owner/member
reconstruction after the structured carrier failed. Concrete alias-template
member-typedef substitution now resolves through
`ParserAliasTemplateInfo::member_typedef`; dependent owner args are preserved
only from that structured carrier.

## Suggested Next

Supervisor/reviewer acceptance for Step 2.4.4.5B. If accepted, the next
coherent implementation packet is Step 2.4.4.6, using the existing
Step 2.4.4.5A direct/qualified/alias-argument carrier coverage as the
guardrail.

## Watchouts

- Dependent alias-template member-typedef results still have to be represented
  in `TypeSpec` compatibility fields because this slice did not add a new
  cross-stage structured projection payload there; the owner/member identity
  feeding that representation now comes from `ParserAliasTemplateInfo::member_typedef`.
- No current Step 2.4.4.6 blocker was found in the delegated parser subset.

## Proof

`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|cpp_parse_local_using_alias_statement_probe_dump|cpp_positive_sema_qualified_member_typedef_functional_cast_frontend_cpp|cpp_(positive_parser|positive_sema|negative_tests))' --output-on-failure) > test_after.log 2>&1`

Passed: 928/928 tests in the delegated subset for Step 2.4.4.5B.
Canonical proof log: `test_after.log`.
