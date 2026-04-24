Status: Active
Source Idea Path: ideas/open/86_parser_alias_template_structured_identity.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Tighten `current_struct_tag` Access

# Current Packet

## Just Finished

Step 3 audit packet completed for the remaining `current_struct_tag_text()` and
raw `current_struct_tag` uses in `parser_types_struct.cpp`, focused on
owner/member registration plus constructor/destructor paths.

Findings:
- `try_parse_record_using_member(...)` and
  `try_parse_record_typedef_member(...)` still gate member-typedef owner
  registration on `active_context_state_.current_struct_tag.empty()`, then pass
  `current_struct_tag_text()` to `register_struct_member_typedef_binding(...)`.
  The owner spelling is intentional member-owner spelling, but the presence
  check is still raw-string-first.
- `is_record_special_member_name(...)` intentionally compares the token lexeme
  against `current_struct_tag_text()` and `struct_source_name`; this preserves
  injected/template-origin spelling for constructor/destructor recognition.
- `try_parse_record_constructor_member(...)` and
  `try_parse_record_destructor_member(...)` still use raw
  `current_struct_tag.empty()` as semantic entry gates before the accessor-based
  special-member comparison. These are the remaining cleanup candidates.
- `begin_record_body_context(...)` and `parse_record_body_with_context(...)`
  still snapshot fallback spelling, but the exact `TextId` restore path now
  lives in the wrapper and remains intentional rollback/fallback state.

## Suggested Next

Next Step 3 code packet: replace the raw `current_struct_tag.empty()` presence
gates in `parser_types_struct.cpp` member typedef registration and
constructor/destructor detection with accessor/TextId-aware checks, preserving
`current_struct_tag_text()` as the owner/injected spelling handed to existing
registration and special-member comparison code.

## Watchouts

- Do not remove the `struct_source_name` comparison in
  `is_record_special_member_name(...)`; it is the fallback/injected spelling
  path for template-origin constructor/destructor names.
- Do not rework `register_struct_member_typedef_binding(...)` storage in this
  Step 3 cleanup. The current member-owner spelling handoff is intentional; only
  the raw-string presence gates need tightening.
- The save/restore fallback string in record-body context is still rollback
  support, not semantic lookup authority.

## Proof

Audit-only packet; no build required and no proof log touched.

Local validation: `git diff --check` passed.
