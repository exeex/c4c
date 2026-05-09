Status: Active
Source Idea Path: ideas/open/155_template_scope_owner_and_member_substitution_domain_keys.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Route Member Typedef Instantiation Through Owner/Member Keys

# Current Packet

## Just Finished

Completed Step 3 member typedef owner/member key routing. Template member
typedef clone registration, selected specialization scans, alias member typedef
resolution, deferred lookup, and direct `Template<Args>::member` parsing now
prefer owner `QualifiedNameKey` plus member `TextId` metadata before rendered
member names, with string conversion left as the no-metadata bridge. Updated
parser coverage for stale rendered member spelling and same-spelling member
TextIds under different concrete owner keys.

## Suggested Next

Next implementation packet target: supervisor should choose whether to run a
review/validation packet for the completed Step 3 and Step 4 slices, or route
the next active plan step if more implementation remains.

## Watchouts

- Do not weaken tests or add named-fixture shortcuts to make a narrow case pass.
- Member typedef name fallback remains only for records without
  `member_typedef_text_ids`; metadata-bearing records should route through the
  member `TextId`.

## Proof

Passed:

`cmake --build --preset default && ctest --test-dir build -R '^(frontend_parser_tests|frontend_parser_lookup_authority_tests|cpp_hir_member_typedef_binding_structured_metadata|cpp_hir_member_typedef_origin_binding_structured_metadata|cpp_hir_member_typedef_realized_owner_structured_metadata|cpp_hir_parser_member_typedef_lookup_structured_metadata|cpp_hir_parser_type_base_alias_member_substitution_structured_metadata|cpp_hir_parser_type_base_member_substitution_structured_metadata)$' --output-on-failure`

Full build and focused ctest output captured in `test_after.log`.
