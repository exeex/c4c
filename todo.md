Status: Active
Source Idea Path: ideas/open/154_parser_sema_qualified_name_text_reparse_retirement.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Retire Ordinary Semantic Reparse Callers

# Current Packet

## Just Finished

Completed Step 2 ordinary qualified resolver packet in
`src/frontend/parser/impl/core.cpp`.

- Added shared ordinary qualified-name helpers that treat complete
  `qualifier_text_ids` as the semantic qualifier carrier and keep
  `qualifier_segments` as a bounded legacy mirror fallback.
- Updated `find_qualified_name_key`, `Parser::qualified_name_key`,
  `resolve_namespace_context`, `resolve_namespace_name`,
  `resolve_qualified_value`, and `resolve_qualified_type` to use the structured
  carrier for shape/iteration and base lookup when available.
- No rendered `A::B::C` splitter was added; mirror fallback still rejects names
  containing `::`.

## Suggested Next

Continue Step 2 with a bounded scan of `core.cpp` callers that project
`QualifiedNameRef` into AST/display state, especially `apply_qualified_name` and
namespace owner fallback, to decide whether any remaining ordinary semantic
branch still uses `qualifier_segments` as authority instead of TextIds.

## Watchouts

- `qualified_name_text`, parser construction, and AST projection still naturally
  use `qualifier_segments` for spelling/display; do not treat those as semantic
  lookup regressions without a concrete ordinary resolution branch.
- The legacy mirror fallback is intentionally find/intern-by-segment only and
  should remain compatibility-only.

## Proof

Passed. Proof log: `test_after.log`.

Command:

`cmake --build --preset default && ctest --test-dir build -R '^(frontend_parser_lookup_authority_tests|frontend_hir_lookup_tests|cpp_hir_parser_member_typedef_lookup_structured_metadata|cpp_hir_parser_type_base_deferred_member_template_origin_structured_metadata|cpp_positive_sema_qualified_member_typedef_functional_cast_frontend_cpp|cpp_positive_sema_namespace_cross_namespace_lookup_parse_cpp)$' -j --output-on-failure`
