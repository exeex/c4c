Status: Active
Source Idea Path: ideas/open/154_parser_sema_qualified_name_text_reparse_retirement.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Retire Ordinary Semantic Reparse Callers

# Current Packet

## Just Finished

Completed Step 2 non-`core.cpp` ordinary `QualifiedNameRef` semantic caller
scan and TextId-first cleanup.

- Scanned focused non-`core.cpp` callers in `declarations.cpp`,
  `expressions.cpp`, `types/declarator.cpp`, `types/base.cpp`,
  `types/template.cpp`, and `types_helpers.hpp` for
  `QualifiedNameRef`, `qualifier_segments`, `resolve_qualified_type/value`,
  and `qualified_name_key` usage.
- Added ordinary qualifier accessors in `types_helpers.hpp` so semantic callers
  can test/count/copy qualifier `TextId`s first, with segment strings retained
  only as bounded legacy mirrors.
- Updated semantic branches in declarations, expressions, declarator, and type
  helper lookup paths that were using `qualifier_segments.empty()` or
  `qualifier_segments.size()` as ordinary lookup authority.
- Retained `qualifier_segments` uses that are syntax-position reconstruction,
  AST/display mirror copying, or compatibility fallback internals; no rendered
  `A::B::C` splitter was added.
- Step 2 is ready for supervisor review.

## Suggested Next

Send Step 2 for reviewer scrutiny against the source idea, with attention to
the retained syntax/display/compatibility `qualifier_segments` uses.

## Watchouts

- `types/template.cpp` still has a branch that calls `resolve_namespace_context`
  when segment mirrors are present and otherwise follows `qualifier_text_ids`
  directly; this is retained because `resolve_namespace_context` is now
  TextId-first for ordinary qualifiers.
- `types/base.cpp` remaining segment-size uses in the focused scan are token
  lookahead math from freshly parsed syntax or syntax reconstruction, not
  semantic lookup authority.
- `declarations.cpp` and expression/declarator AST projection still preserve
  segment mirrors beside TextIds for downstream compatibility and display.
- No blocker found in non-`core.cpp` ordinary semantic callers.

## Proof

Passed. Proof log: `test_after.log`.

Command:

`cmake --build --preset default && ctest --test-dir build -R '^(frontend_parser_lookup_authority_tests|frontend_hir_lookup_tests|cpp_hir_parser_member_typedef_lookup_structured_metadata|cpp_hir_parser_type_base_deferred_member_template_origin_structured_metadata|cpp_positive_sema_qualified_member_typedef_functional_cast_frontend_cpp|cpp_positive_sema_namespace_cross_namespace_lookup_parse_cpp)$' -j --output-on-failure`
