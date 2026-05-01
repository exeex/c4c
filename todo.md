# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.4.3
Current Step Title: Prove Qualified Key Or Namespace-Context Lookup

## Just Finished

Step 2.4.3 converted the `parse_base_type` member-typedef recursion helper away
from scoped rendered `TextId` typedef lookup. The
`lookup_struct_member_typedef_recursive_for_type` route now resolves owner
typedefs from existing `TypeSpec` metadata: `tag_text_id`, qualifier
`TextId`s/global qualification through `QualifiedNameKey`, or
`namespace_context_id` through `struct_typedef_key_in_context`. Plain
unqualified owner names still use visible typedef lookup.

The same helper now uses `TypeSpec` qualified metadata for structured record
definition lookup instead of reparsing `ts.tag` as qualified text on that route.

## Suggested Next

Have the supervisor review and commit the completed Step 2.4.3 slice, then
select the next Step 2.4 packet from the active plan.

## Watchouts

- Do not delete the `owner::member` mirror, delete APIs, or claim storage
  narrowing from this packet.
- Continue rejecting helpers that accept rendered `owner::member` text,
  `std::string`, or `std::string_view` qualified text and parse it back into
  owner/member identity.
- `find_typedef_type(TextId)` scoped-string handling still exists outside this
  packet, but the targeted `lookup_struct_member_typedef_recursive_for_type`
  consumer no longer calls it for scoped rendered owner names.
- Instantiated member typedefs such as `ns::holder_T_int::type` still need
  concrete owner-member `TextId` or equivalent domain metadata before broader
  cleanup.
- HIR-only rendered lookup cleanup remains out of scope for idea 139 and
  belongs to idea 140.

## Proof

Delegated proof was run and is preserved in `test_after.log`:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|cpp_positive_sema_(template_member_type_direct_parse_cpp|template_member_typedef_cache_roundtrip_parse_cpp|record_member_typedef_using_parse_cpp|template_specialization_member_typedef_trait_parse_cpp|template_variable_member_typedef_normalization_runtime_cpp|template_variable_alias_member_typedef_runtime_cpp|inherited_type_alias_base_member_lookup_parse_cpp|namespaced_inherited_type_alias_base_member_lookup_parse_cpp))$' > test_after.log 2>&1`

Result: passed, 9/9 tests.
