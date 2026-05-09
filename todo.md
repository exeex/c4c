Status: Active
Source Idea Path: ideas/open/154_parser_sema_qualified_name_text_reparse_retirement.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Retire Ordinary Semantic Reparse Callers

# Current Packet

## Just Finished

Completed Step 2 first semantic-key packet in
`src/frontend/parser/impl/core.cpp`.

- `find_qualified_name_key` and `Parser::qualified_name_key` now use
  `base_text_id` and complete `qualifier_text_ids` as the direct semantic
  identity path.
- `base_name` / `qualifier_segments` fallback is bounded behind explicitly named
  legacy mirror helpers/comments and rejects rendered `A::B::C` spelling
  instead of splitting it.
- Added `frontend_parser_lookup_authority_tests` coverage proving complete
  qualifier/base `TextId`s win over stale string mirrors.

## Suggested Next

Continue Step 2 with a bounded packet on the remaining ordinary semantic
callers in `src/frontend/parser/impl/core.cpp`: make namespace/type/value
resolution predicates distinguish structured `qualifier_text_ids` from legacy
`qualifier_segments` mirrors where needed, without changing display/debug
projection or injected syntax reconstruction.

## Watchouts

- `resolve_qualified_value`, `resolve_qualified_type`,
  `resolve_namespace_context`, and `resolve_namespace_name` still use
  `qualifier_segments.empty()` / string-vector iteration as ordinary lookup
  shape tests; that is the next likely semantic mirror dependency.
- The legacy fallback intentionally remains for constructed refs with missing
  ids; it should stay compatibility-only and must not become a rendered-name
  splitter.
- `declarator.cpp` was not needed for this slice.

## Proof

Passed. Proof log: `test_after.log`.

Command:

`cmake --build --preset default && ctest --test-dir build -R '^(frontend_parser_lookup_authority_tests|frontend_hir_lookup_tests|cpp_hir_parser_member_typedef_lookup_structured_metadata|cpp_hir_parser_type_base_deferred_member_template_origin_structured_metadata|cpp_positive_sema_qualified_member_typedef_functional_cast_frontend_cpp|cpp_positive_sema_namespace_cross_namespace_lookup_parse_cpp)$' -j --output-on-failure`
