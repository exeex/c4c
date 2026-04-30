# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.4.4.5A.1
Current Step Title: Construct Alias-Template Member-Typedef Carrier Before TypeSpec Flattening

## Just Finished

Step 2.4.4.5A.1 is implemented as a producer-only parser slice.
`ParserAliasTemplateInfo` now carries a structured
`ParserAliasTemplateMemberTypedefInfo` for RHS forms like
`typename Owner<Args>::member`, populated by a tentative parser-structure pass
before normal `TypeSpec` parsing/flattening. The carrier preserves the owner
`QualifiedNameKey`, parsed/substitutable owner template arguments, and member
`TextId`. A focused `frontend_parser_tests` case proves the carrier remains
intact after the rendered/deferred `TypeSpec` spelling is deliberately drifted.

## Suggested Next

Execute Step 2.4.4.5A.2: consume the structured alias-template member-typedef
carrier during alias instantiation/substitution, while keeping bridge deletion
out of scope until Step 2.4.4.5B.

## Watchouts

- The existing rendered/deferred `TypeSpec` bridge remains in place because
  deleting it caused the expected bridge-regression failures; Step 2.4.4.5B
  still owns bridge deletion.
- The new carrier is not seeded from `tpl_struct_origin`,
  `deferred_member_type_name`, `TypeSpec::tag`, `qualified_name_from_text`,
  `qualified_alias_name`, rendered owner/member spellings,
  `TemplateArgRef::debug_text`, saved RHS token reparsing, or a local
  alias-of-alias parser.
- The current producer parser accepts structured `typename Owner<Args>::member`
  shapes; Step 2.4.4.5A.2 should preserve the same route discipline when
  mapping alias actuals onto `member_typedef.owner_args`.

## Proof

`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|cpp_parse_local_using_alias_statement_probe_dump|cpp_positive_sema_qualified_member_typedef_functional_cast_frontend_cpp|cpp_(positive_parser|positive_sema|negative_tests))' --output-on-failure) > test_after.log 2>&1`

Passed: 928/928 tests in the delegated subset. `test_after.log` is the
canonical proof log for this packet.
