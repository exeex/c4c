# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.4.4.5A.1
Current Step Title: Construct Alias-Template Member-Typedef Carrier Before TypeSpec Flattening

## Just Finished

Step 2.4.4.5A.1 moved alias-template member-typedef ownership to the
pre-flattening `ParserAliasTemplateMemberTypedefInfo` producer path for
`typename Owner<Args>::member`. The using-alias path now preserves structured
owner `QualifiedNameKey`, parsed owner argument refs, and member `TextId`
before `parse_type_name()` flattens the RHS.

The old local token-range parser for alias-template member typedefs was
removed. Compatibility `TypeSpec` fields are projected only from the structured
carrier when the normal type parse lacks a template origin; the carrier is not
seeded from `tpl_struct_origin`, `deferred_member_type_name`, `TypeSpec::tag`,
rendered qualified names, `qualified_alias_name`, or debug text.

## Suggested Next

Next executor packet can proceed to Step 2.4.4.5A.2 bridge resolution work if
selected by the supervisor. Keep bridge deletion out of scope until the
structured carrier route is reviewed and accepted.

## Watchouts

- The `TypeSpec` compatibility projection in the alias-template using-alias
  path is intentionally sourced from `ParserAliasTemplateMemberTypedefInfo`;
  do not replace it with token spelling reconstruction.
- `record_member_typedef_key_in_context` and
  `register_dependent_record_member_typedef_binding` still have live
  dependent/template bridge users and remain parked for later structured
  carrier steps.
- HIR member-typedef resolver cleanup remains out of scope for idea 139 and
  belongs to idea 140.

## Proof

Delegated proof:

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_' >> test_after.log 2>&1`

Result: passed. `test_after.log` contains the build output and 880/880 passing
`cpp_positive_sema_` tests, including
`cpp_positive_sema_template_alias_member_typedef_structured_carrier_runtime_cpp`.
