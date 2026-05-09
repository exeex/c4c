Status: Active
Source Idea Path: ideas/open/154_parser_sema_qualified_name_text_reparse_retirement.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Retire Ordinary Semantic Reparse Callers

# Current Packet

## Just Finished

Completed Step 2 `core.cpp` projection/owner-fallback packet in
`src/frontend/parser/impl/core.cpp`.

- Scanned remaining `core.cpp` `QualifiedNameRef` projection callers after the
  ordinary resolver helper work.
- Changed `apply_qualified_name` namespace owner fallback to treat
  `qualifier_text_ids` as the authority when trimming the owner qualifier,
  with `qualifier_segments` and `qualifier_symbol_ids` only resized as mirrors.
- Left `qualified_name_text`, parser construction, and AST/debug spelling
  projection on `qualifier_segments`; those are retained display/compatibility
  paths, not ordinary semantic lookup authority.
- No rendered `A::B::C` splitter was added.

## Suggested Next

Continue Step 2 with a supervisor-selected bounded scan of non-`core.cpp`
ordinary `QualifiedNameRef` callers, or send Step 2 for review if the remaining
projection-only uses are acceptable.

## Watchouts

- `apply_qualified_name` still populates AST spelling arrays from
  `qualifier_segments` because downstream AST/HIR display and compatibility
  paths expect that shape; semantic consumers should use the paired TextIds.
- The legacy mirror fallback remains intentionally find/intern-by-segment only
  and should remain compatibility-only.
- No blocker found in `core.cpp`: the remaining ordinary semantic branches
  inspected now prefer TextIds when present.

## Proof

Passed. Proof log: `test_after.log`.

Command:

`cmake --build --preset default && ctest --test-dir build -R '^(frontend_parser_lookup_authority_tests|frontend_hir_lookup_tests|cpp_hir_parser_member_typedef_lookup_structured_metadata|cpp_hir_parser_type_base_deferred_member_template_origin_structured_metadata|cpp_positive_sema_qualified_member_typedef_functional_cast_frontend_cpp|cpp_positive_sema_namespace_cross_namespace_lookup_parse_cpp)$' -j --output-on-failure`
