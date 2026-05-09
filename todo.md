Status: Active
Source Idea Path: ideas/open/154_parser_sema_qualified_name_text_reparse_retirement.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Demote Qualified Member-Type String Splitting

# Current Packet

## Just Finished

Completed Step 3 qualified member-type string splitter demotion.

- Scanned `split_qualified_member_type_name` and its adjacent member-type
  lookup helpers in `types_helpers.hpp` plus the active declarator member-type
  lookup paths.
- Found no active callers of `split_qualified_member_type_name`; its private
  rendered-text parser `qualified_name_from_text` was only used by that
  splitter.
- Removed both unused helpers from `types_helpers.hpp`.
- Retained existing semantic member-type lookup paths because they already use
  structured `QualifiedNameRef`, `QualifiedNameKey`, owner keys, and member
  `TextId`s; no rendered `A::B::C` splitter was added.

## Suggested Next

Proceed to the next plan step by inspecting any remaining rendered qualified
name reparse or compatibility paths outside this member-type splitter.

## Watchouts

- No active member-type caller still requires rendered string splitting.
- `append_qualified_name_tokens` remains as an injected-token compatibility
  emitter, not as semantic lookup authority.

## Proof

Passed. Proof log: `test_after.log`.

Command:

`cmake --build --preset default && ctest --test-dir build -R '^(frontend_parser_lookup_authority_tests|frontend_hir_lookup_tests|cpp_hir_parser_member_typedef_lookup_structured_metadata|cpp_hir_parser_type_base_deferred_member_template_origin_structured_metadata|cpp_positive_sema_qualified_member_typedef_functional_cast_frontend_cpp|cpp_positive_sema_namespace_cross_namespace_lookup_parse_cpp)$' -j --output-on-failure`
