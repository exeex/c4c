# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.4.4.5A.1
Current Step Title: Construct Alias-Template Member-Typedef Carrier Before TypeSpec Flattening

## Just Finished

Reviewer rejected the attempted Step 2.4.4.5A route in
`review/step2_4_5a_alias_template_carrier_review.md`. The implementation was
reverted, so the remaining lifecycle action is route repair. The rejected
attempt added real carrier storage, but seeded and recovered it from
rendered/deferred `TypeSpec` strings (`tpl_struct_origin`,
`deferred_member_type_name`, `TypeSpec::tag`, `qualified_name_from_text`,
`qualified_alias_name`, and `debug_text`) plus a narrow local alias-of-alias
token parser. That is route drift for the active source idea because semantic
owner/member/argument identity still came from rendered spelling.

`plan.md` now splits the old Step 2.4.4.5A into smaller checkpoints:
Step 2.4.4.5A.1 constructs the alias-template member-typedef carrier before
`TypeSpec` flattening, Step 2.4.4.5A.2 consumes it during alias instantiation,
and Step 2.4.4.5A.3 reviews the route before Step 2.4.4.5B bridge deletion.

## Suggested Next

Execute Step 2.4.4.5A.1 only: add a structured alias member-typedef carrier to
`ParserAliasTemplateInfo` or an equivalent parser/Sema alias-template metadata
surface for `typename Owner<Args>::member`. Populate it directly from parser
structures while the alias RHS is parsed, before rendered/deferred `TypeSpec`
flattening. Preserve owner `QualifiedNameKey`, argument refs/keys, and member
`TextId`. Do not consume the carrier for bridge deletion in this packet except
for minimal producer proof; Step 2.4.4.5A.2 owns normal alias-instantiation
consumption.

## Watchouts

- Do not seed or recover the carrier from `tpl_struct_origin`,
  `deferred_member_type_name`, `TypeSpec::tag`, `qualified_name_from_text`,
  `qualified_alias_name`, rendered `Owner::member`, rendered `mangled`
  spelling, `std::string`, `std::string_view`, fallback spelling, or
  `TemplateArgRef::debug_text`.
- Do not add a narrow local alias-of-alias parser that reparses saved RHS token
  text or concatenates token spelling into semantic identity.
- Qualified alias-template paths such as `ns::alias_t<...>` must carry a
  structured qualified alias key or equivalent parser key directly; a rendered
  qualified alias spelling relay is the rejected route.
- Deleting the dependent/template bridge still belongs to Step 2.4.4.5B after
  Steps 2.4.4.5A.1 through 2.4.4.5A.3 land and pass review. The known bridge
  regression coverage remains
  `cpp_positive_sema_eastl_slice7_piecewise_ctor_parse_cpp`,
  `cpp_positive_sema_step3_timeout_probe_baseline_parse_cpp`,
  `cpp_positive_sema_tuple_element_alias_mix_parse_cpp`, and
  `cpp_positive_sema_template_variable_alias_member_typedef_runtime_cpp`.

## Proof

`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|cpp_parse_local_using_alias_statement_probe_dump|cpp_positive_sema_qualified_member_typedef_functional_cast_frontend_cpp|cpp_(positive_parser|positive_sema|negative_tests))' --output-on-failure) > test_after.log 2>&1`

No acceptance proof is claimed for the rejected route. The next executor should
produce fresh `test_after.log` for the delegated Step 2.4.4.5A.1 proof command.
