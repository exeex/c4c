# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Remove Parser Rendered-String Semantic Lookup Routes

## Just Finished

Step 2 removed `parse_base_type` member-typedef authority from rendered
`resolved_tag` lookup: `lookup_struct_member_typedef_recursive_for_type` no
longer falls back to `struct_tag_def_map.find(resolved_tag)` after typed record
identity misses, and no longer resolves `resolved_tag + "::" + member` through
flat typedef storage. Focused parser tests now reject rendered-only owner/tag
and rendered `owner::member` storage while preserving `record_def` and direct
member typedef arrays. This packet finished the dirty slice by reattaching the
existing concrete template instantiation `record_def` when direct template
instantiation dedup has already fired, so deferred `typename
ns::holder<int>::type` arguments normalize to `int` in the static-member
constexpr/template-variable path without restoring rendered member lookup.

## Suggested Next

Supervisor should review and commit this Step 2 slice, then choose the next
rendered-string semantic lookup route from the active plan.

## Watchouts

The member typedef resolver still intentionally rejects rendered-only owner
recovery and rendered `owner::member` typedef storage. The new reattachment is
limited to the direct template-instantiation path so later lookups can use
`TypeSpec::record_def` on the returned owner type.

## Proof

Passed. Ran the delegated proof command:
`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|cpp_(positive_parser|positive_sema|negative_tests))' --output-on-failure) > test_after.log 2>&1`.
Build passed and CTest reported 927/927 passing. Proof log:
`test_after.log`.
