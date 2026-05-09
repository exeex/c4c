Status: Active
Source Idea Path: ideas/open/154_parser_sema_qualified_name_text_reparse_retirement.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Bound Token Injection Helpers

# Current Packet

## Just Finished

Completed Step 4 token-injection helper bounding.

- Audited `append_qualified_name_tokens` and found only two direct callers:
  the legacy `TypeSpec` tag fallback inside `append_typespec_reparse_tokens`
  and the template-struct injected-parse spelling in
  `instantiate_template_struct_via_injected_parse`.
- Renamed the helper to `append_qualified_name_compatibility_tokens` and
  documented that it is only a compatibility spelling emitter for injected
  parser recovery paths.
- Confirmed structured carriers remain the lookup authority:
  `append_typespec_metadata_name_tokens` is preferred before legacy tag
  emission, and template instantiation still keys identity through structured
  template keys after injected syntax reconstruction.
- Added no rendered `A::B::C` splitter and found no token-injection caller that
  still acts as semantic lookup authority.

## Suggested Next

Proceed to supervisor review or the next planned retirement slice for remaining
rendered qualified-name compatibility paths outside the now-bounded injected
token emitter.

## Watchouts

- The compatibility emitter still tokenizes legacy rendered spellings only for
  reparsing through `parse_injected_base_type`; future semantic lookup work
  should continue to use `TypeSpec` metadata, `QualifiedNameRef`, or
  `QualifiedNameKey` instead.
- `review/step2_qualified_name_textid_route_review.md` remains an existing
  untracked transient artifact and was not touched.

## Proof

Passed. Proof log: `test_after.log`.

Command:

`cmake --build --preset default && ctest --test-dir build -R '^(frontend_parser_lookup_authority_tests|frontend_hir_lookup_tests|cpp_hir_parser_member_typedef_lookup_structured_metadata|cpp_hir_parser_type_base_deferred_member_template_origin_structured_metadata|cpp_positive_sema_qualified_member_typedef_functional_cast_frontend_cpp|cpp_positive_sema_namespace_cross_namespace_lookup_parse_cpp)$' -j --output-on-failure`
