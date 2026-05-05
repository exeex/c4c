# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Delete TypeSpec Tag And Validate

## Just Finished

Step 6's C++ positive/Sema parser fallout repair improved the
`cpp_positive_sema_` baseline without reintroducing `TypeSpec::tag`. Qualified
template-id type parsing now accepts qualified alias templates before `<...>`
and projects the structured `ParserAliasTemplateInfo::member_typedef` /
`ParserAliasTemplateMemberTypedefInfo` carrier into a `TypeSpec` for declaration
and parameter type contexts. This repairs qualified/template alias member
typedef cases such as `ns::alias_of_alias_t<int, short>` and
`carrier_probe::pick_second_t<int, short>` without `_t` spelling inference or
rendered-string semantic lookup.

The same qualified type parser now keeps unresolved qualified template-id type
heads such as `ns::holder<T>` on the type side by carrying the structured
qualified owner key plus parsed template arguments instead of leaving `<...>`
for expression parsing.

## Suggested Next

Next packet should target the remaining pre-existing C++ positive/Sema fallout
left in `test_after.log`, dominated by EASTL/template-owner/member access and
copy/move/range-for failures.

## Watchouts

- Do not reintroduce `TypeSpec::tag` or rendered-string semantic lookup.
- Do not infer alias-template targets from `_t` spelling, debug text,
  `tag_ctx`, rendered names, or module dump strings.
- The delegated proof is still red because 46 baseline failures remain, but the
  failed set only shrank: 51 failures before, 46 after, with no new failures.
- The repaired family includes
  `cpp_positive_sema_template_alias_member_typedef_dependent_ref_runtime_cpp`,
  `cpp_positive_sema_template_alias_member_typedef_reordered_owner_runtime_cpp`,
  `cpp_positive_sema_template_alias_member_typedef_structured_carrier_runtime_cpp`,
  `cpp_positive_sema_qualified_template_unresolved_param_type_parse_cpp`, and
  adjacent `cpp_positive_sema_sfinae_template_parameter_patterns_parse_cpp`.

## Proof

Delegated proof command:
`cmake --build build && ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_' > test_after.log 2>&1`

Result: improved red baseline.

`cmake --build build` passed. The `cpp_positive_sema_` subset now reports
838/884 passing and 46 failing, compared with `test_before.log` at 833/884
passing and 51 failing. Fixed tests: the three qualified/template alias member
typedef cases, `cpp_positive_sema_qualified_template_unresolved_param_type_parse_cpp`,
and `cpp_positive_sema_sfinae_template_parameter_patterns_parse_cpp`. No new
failures were introduced.

Canonical proof log: `test_after.log`.
