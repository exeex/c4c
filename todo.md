Status: Active
Source Idea Path: ideas/open/86_parser_alias_template_structured_identity.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Tighten `current_struct_tag` Access

# Current Packet

## Just Finished

Step 3 code packet completed for `parser_types_struct.cpp` record-body
`current_struct_tag` save/restore paths.

`parse_record_body_with_context(...)` now snapshots both
`active_context_state_.current_struct_tag_text_id` and fallback spelling before
entering the record body. `finish_record_body_context(...)` restores through a
TextId-aware helper, and the wrapper performs the final rehydration from the
saved TextId plus fallback spelling after the closing brace is consumed.

Constructor/destructor injected-name comparisons and `struct_source_name`
fallback spelling remain unchanged.

## Suggested Next

Next Step 3 packet: audit the remaining `current_struct_tag_text()` and
spelling-keyed owner/member registration uses in `parser_types_struct.cpp` and
decide whether Step 3 needs one more bounded cleanup or can advance.

## Watchouts

- Do not collapse `current_struct_tag` to `TextId` only yet:
  constructor/destructor matching still intentionally compares against
  `struct_source_name` for template-origin/injected spellings.
- `register_struct_member_typedef_binding(...)` remains spelling-keyed; changing
  it is a separate owner-registration packet, not the first safe cleanup.
- `finish_record_body_context(...)` keeps its existing signature; the exact
  saved TextId is preserved and restored by `parse_record_body_with_context(...)`.

## Proof

Delegated proof passed:
`bash -lc 'set -o pipefail; cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '\''^frontend_parser_tests$'\'' | tee test_after.log'`

Proof log path: `test_after.log`
