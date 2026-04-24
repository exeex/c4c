Status: Active
Source Idea Path: ideas/open/86_parser_alias_template_structured_identity.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Tighten `current_struct_tag` Access

# Current Packet

## Just Finished

Step 3 code packet completed for `parser_declarations.cpp` owner-scope
`current_struct_tag` save/restore paths.

The out-of-class operator, out-of-class constructor, and qualified declarator
owner-scope paths now snapshot both
`active_context_state_.current_struct_tag_text_id` and fallback spelling before
entering the qualified owner scope. They restore through a shared accessor-based
rehydration helper instead of restoring from the raw string alone.

`qualified_owner`, `qualified_owner_tag`, and `owner_struct_tag` spelling
behavior remains unchanged for emitted/member names and template-scope owner
relabeling.

## Suggested Next

Next Step 3 packet: audit and tighten `parser_types_struct.cpp` record-body
`current_struct_tag` save/restore around `begin_record_body_context(...)`,
`finish_record_body_context(...)`, and `parse_record_body_with_context(...)`,
while preserving constructor/destructor injected-name comparisons and
`struct_source_name` fallback spelling.

## Watchouts

- Do not collapse `current_struct_tag` to `TextId` only yet:
  constructor/destructor matching still intentionally compares against
  `struct_source_name` for template-origin/injected spellings.
- `register_struct_member_typedef_binding(...)` remains spelling-keyed; changing
  it is a separate owner-registration packet, not the first safe cleanup.
- `parser_declarations.cpp` still intentionally feeds qualified owner spelling
  into emitted function names and `owner_struct_tag`; this packet only changed
  context restoration metadata.

## Proof

Delegated proof passed:
`bash -lc 'set -o pipefail; cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '\''^frontend_parser_tests$'\'' | tee test_after.log'`

Proof log path: `test_after.log`
