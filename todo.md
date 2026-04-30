# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Remove Parser Rendered-String Semantic Lookup Routes

## Just Finished

Step 2 qualified declarator member-typedef cleanup removed rendered owner/member
lookup authority from `parse_dependent_typename_specifier(...)`:

- replaced owner traversal through rendered `owner_tag`,
  `find_typedef_type(find_parser_text_id(owner_tag))`, and rendered
  `struct_tag_def_map.find(...)` fallbacks with
  `QualifiedNameRef`-based structured owner resolution;
- removed nested owner recovery through rendered field tags and rendered
  qualified node names when `TypeSpec::record_def` is missing;
- removed the rendered `scoped_name` member typedef fallback so direct
  `owner->member_typedef_*` metadata is the member typedef authority;
- added parser coverage showing legacy rendered owner typedef/tag storage and
  rendered `owner::member` typedef storage alone do not authorize member typedef
  lookup, while namespace-context record metadata plus direct member typedef
  arrays still works.

## Suggested Next

Next executor packet can continue Step 2 by auditing remaining parser
qualified-type helpers outside this declarator member-typedef route, especially
the compatibility comments in `types_helpers.hpp` that still explicitly allow
rendered direct record-tag fallback for broader type-start probes.

## Watchouts

Older parser fixtures that expected member typedef lookup through a tag-only
owner alias now need `TypeSpec::record_def` or namespace-context record
metadata. That is intentional for this route; the implementation does not add
a replacement rendered-string wrapper.

## Proof

`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|cpp_(positive_parser|positive_sema|negative_tests))' --output-on-failure) > test_after.log 2>&1`

Result: passed, 927/927 tests green. Proof log: `test_after.log`.
