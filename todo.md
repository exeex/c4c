# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.4.4.5A.3
Current Step Title: Review Alias-Template Carrier Route Before Bridge Deletion

## Just Finished

Step 2.4.4.5A.3 now has focused `frontend_parser_tests` coverage for
qualified-alias and alias-of-alias member-typedef carrier consumption. The new
qualified `ns::Alias<int>` case and alias-of-alias argument case both corrupt
the rendered/deferred `TypeSpec` owner/member spelling and still resolve
through the structured `ParserAliasTemplateInfo::member_typedef` carrier using
the owner `QualifiedNameKey`, member `TextId`, and structured owner args.
Step 2.4.4.5A.3 is ready for supervisor/reviewer acceptance.

## Suggested Next

Supervisor/reviewer acceptance for Step 2.4.4.5A.3. If accepted, the next
coherent implementation packet is Step 2.4.4.5B bridge deletion, with the
existing Step 2.4.4.5A carrier coverage retained as the guardrail.

## Watchouts

- The structured consumer intentionally bails out for owner args that still
  mention active template-scope parameters so dependent alias-of-alias cases
  keep using the existing dependent/template bridge until concrete
  instantiation.
- The rendered/deferred `TypeSpec` bridge and alias-name-derived `_t` fallback
  remain reachable only after the structured carrier route fails; Step
  2.4.4.5B still owns bridge deletion.
- The consumer does not infer carrier identity from `tpl_struct_origin`,
  `deferred_member_type_name`, `TypeSpec::tag`, rendered owner/member spelling,
  `TemplateArgRef::debug_text`, saved RHS token reparsing, or a local
  alias-of-alias parser.

## Proof

`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|cpp_parse_local_using_alias_statement_probe_dump|cpp_positive_sema_qualified_member_typedef_functional_cast_frontend_cpp|cpp_(positive_parser|positive_sema|negative_tests))' --output-on-failure) > test_after.log 2>&1`

Passed: 928/928 tests in the delegated subset for Step 2.4.4.5A.3.
Canonical proof log: `test_after.log`.
