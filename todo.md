Status: Active
Source Idea Path: ideas/open/154_parser_sema_qualified_name_text_reparse_retirement.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Audit QualifiedNameRef Display Mirrors

# Current Packet

## Just Finished

Completed Step 5 QualifiedNameRef display mirror audit.

- Audited direct parser uses of `QualifiedNameRef::qualifier_segments` and
  `base_name`; retained display/debug/syntax uses are token-position math,
  rendered `qualified_name_text`, parsed spelling construction, compatibility
  TypeSpec token reconstruction, and arena display copies.
- Fixed semantic carrier propagation in `Parser::apply_qualified_name` so Node
  qualifier metadata is sized and copied from the paired qualifier TextIds when
  present, with display segments only as bounded legacy compatibility fallback.
- Fixed static-member expression `member_qn` construction to copy qualifier
  TextIds through `qualified_name_ordinary_qualifier_count` and helper accessors
  instead of iterating display mirrors directly.
- Confirmed remaining semantic lookup paths use `QualifiedNameKey`,
  `NamePathTable`/context keys, base/qualifier TextIds, or symbol/Node metadata
  before display spelling; no rendered `A::B::C` splitter was added.

## Suggested Next

Proceed to Step 6 closure review and broader proof selection for the retained
compatibility/display paths.

## Watchouts

- Retained mirror reads in `peek_qualified_name`, declaration/type-start
  lookahead, and declarator parsing are syntax/display carriers paired with
  `base_text_id` and `qualifier_text_ids` from parsed tokens.
- Retained fallback helpers (`find_legacy_mirrored_name_text_id`,
  `intern_legacy_mirrored_name_text_id`, and TypeSpec mirror bridges) are
  bounded to unqualified segment strings and reject rendered qualified names.
- `review/step2_qualified_name_textid_route_review.md` remains an existing
  untracked transient artifact and was not touched.

## Proof

Passed. Proof log: `test_after.log`.

Command:

`cmake --build --preset default && ctest --test-dir build -R '^(frontend_parser_lookup_authority_tests|frontend_hir_lookup_tests|cpp_hir_parser_member_typedef_lookup_structured_metadata|cpp_hir_parser_type_base_deferred_member_template_origin_structured_metadata|cpp_positive_sema_qualified_member_typedef_functional_cast_frontend_cpp|cpp_positive_sema_namespace_cross_namespace_lookup_parse_cpp)$' -j --output-on-failure`
