Status: Active
Source Idea Path: ideas/open/86_parser_alias_template_structured_identity.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Tighten `last_using_alias_name` Around Structured Alias Identity

# Current Packet

## Just Finished

Step 3 is exhausted. The final code packet completed for
`parser_types_struct.cpp`: member typedef
registration and constructor/destructor entry gates now use
`current_struct_tag_text().empty()` instead of direct
`active_context_state_.current_struct_tag.empty()` checks.

The existing owner and special-member spelling handoff remains unchanged:
`current_struct_tag_text()` is still passed to
`register_struct_member_typedef_binding(...)`, and
`is_record_special_member_name(...)` still compares against
`current_struct_tag_text()` plus `struct_source_name`.

## Suggested Next

Begin Step 4 with an inventory-only packet for `last_using_alias_name` active
context handoff sites. Classify remaining consumers of
`last_using_alias_name_text()`, `last_using_alias_name_text_id`, and
`last_using_alias_key` in:

- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/parser_declarations.cpp`
- `src/frontend/parser/parser_types_base.cpp`
- `src/frontend/parser/parser_types_template.cpp`

The packet should identify the first safe code change that prefers
`last_using_alias_key` or `last_using_alias_name_text_id` for semantic alias
identity while keeping rendered alias spelling only as fallback, diagnostic, or
compatibility bridge data.

## Watchouts

- Do not remove the `struct_source_name` comparison in
  `is_record_special_member_name(...)`; it is the fallback/injected spelling
  path for template-origin constructor/destructor names.
- Do not rework `register_struct_member_typedef_binding(...)` storage in this
  Step 3 cleanup. The current member-owner spelling handoff is intentional; only
  the raw-string presence gates need tightening.
- No direct `active_context_state_.current_struct_tag.empty()` gates remain in
  `parser_types_struct.cpp`; remaining raw `current_struct_tag` state in this
  file is save/restore fallback support.
- Do not rework alias-template storage beyond the Step 4 active-context
  handoff boundary.
- Do not reopen completed template-struct primary/specialization/instantiation
  lookup cleanup.

## Proof

Delegated proof passed:

`bash -lc 'set -o pipefail; cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '\''^frontend_parser_tests$'\'' | tee test_after.log'`

Local validation: `git diff --check` passed.
Proof log: `test_after.log`.
