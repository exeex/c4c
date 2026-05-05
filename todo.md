# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Delete TypeSpec Tag And Validate

## Just Finished

Step 6's parser-unit regression from the qualified alias-template parsing slice
is repaired without reintroducing `TypeSpec::tag`. The
`try_parse_qualified_base_type` fallback no longer accepts unknown
namespace-qualified template ids such as `ns::Missing<int>` from spelling alone.
Known qualified alias templates still resolve through the structured
`ParserAliasTemplateInfo::member_typedef` /
`ParserAliasTemplateMemberTypedefInfo` carrier, and concrete alias-template
member typedefs can resolve immediately through the selected structured owner
member typedef.

Unresolved qualified template-id type heads are now accepted only when the
template-id is dependent through parsed template arguments or an active template
type-parameter scope, preserving `ns::holder<T>` parameter parsing while
rejecting non-dependent unknown qualified template ids.

## Suggested Next

Next packet can return to the remaining C++ positive/Sema fallout after this
guard repair is reviewed and committed.

## Watchouts

- Do not reintroduce `TypeSpec::tag` or rendered-string semantic lookup.
- Do not infer alias-template targets from `_t` spelling, debug text,
  `tag_ctx`, rendered names, or module dump strings.
- Keep the unknown-template guard dependent/structured-authority based; do not
  restore a fallback that builds `TB_STRUCT` from qualified spelling alone.
- The alias-template path must continue to prefer parser-owned structured
  member-typedef metadata over stale rendered/deferred alias `TypeSpec`
  spelling.

## Proof

Delegated proof command:
`cmake --build build && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|cpp_positive_sema_template_alias_member_typedef_dependent_ref_runtime_cpp|cpp_positive_sema_template_alias_member_typedef_reordered_owner_runtime_cpp|cpp_positive_sema_template_alias_member_typedef_structured_carrier_runtime_cpp|cpp_positive_sema_qualified_template_unresolved_param_type_parse_cpp|cpp_positive_sema_sfinae_template_parameter_patterns_parse_cpp)$' > test_after.log 2>&1`

Result: passed.

`cmake --build build` passed. `frontend_parser_tests` passed again, and the
five positive/Sema tests fixed by the previous slice remained passing:
`cpp_positive_sema_template_alias_member_typedef_dependent_ref_runtime_cpp`,
`cpp_positive_sema_template_alias_member_typedef_reordered_owner_runtime_cpp`,
`cpp_positive_sema_template_alias_member_typedef_structured_carrier_runtime_cpp`,
`cpp_positive_sema_qualified_template_unresolved_param_type_parse_cpp`, and
`cpp_positive_sema_sfinae_template_parameter_patterns_parse_cpp`.

Canonical proof log: `test_after.log`.
