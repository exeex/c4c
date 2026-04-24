Status: Active
Source Idea Path: ideas/open/86_parser_alias_template_structured_identity.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Tighten `current_struct_tag` Access

# Current Packet

## Just Finished

Step 3 inventory packet completed for `current_struct_tag` active-context call
sites across the requested parser files.

Classification:

- Structured semantic gate: `parser_declarations.cpp` incomplete-object checks
  already compare `active_context_state_.current_struct_tag_text_id` plus
  `current_struct_tag_text()` against the candidate tag; this is a structured
  same-visible-type-name gate with spelling only as fallback.
- Structured semantic gate: `parser_types_base.cpp` declaration and postfix
  probes use `current_struct_tag_text()` to decide whether to admit
  `Identifier<...>` as a recoverable type and to try `Current::member` known
  function lookup. These are context gates, not storage authority.
- Structured semantic gate: `parser_expressions.cpp` qualified operator-call
  collapse uses `current_struct_tag_text()` after a non-empty context check and
  compares it with visible typedef resolution for current-object member calls.
- Fallback spelling / member-owner registration:
  `parser_types_struct.cpp` registers record-scope `using` and `typedef`
  aliases under `current_struct_tag_text()`. The `TextId` accessor is used, but
  the downstream registration key remains spelling-shaped.
- Injected/member-owner spelling: `parser_types_struct.cpp`
  `is_record_special_member_name(...)`, constructor/destructor member parsing,
  and record-body setup compare constructor/destructor lexemes against
  `current_struct_tag_text()` and `struct_source_name` to support injected
  template-origin spellings.
- Snapshot/rollback and injected/member-owner spelling:
  `parser_types_struct.cpp` `begin_record_body_context(...)`,
  `finish_record_body_context(...)`, and `parse_record_body_with_context(...)`
  save/restore `active_context_state_.current_struct_tag` while `set_`/`clear_`
  maintain the mirrored `TextId`.
- Snapshot/rollback and injected/member-owner spelling:
  `parser_declarations.cpp` out-of-class operator, constructor, and qualified
  member function paths save the raw string, set `qualified_owner`, use it for
  template-scope owner relabeling, and restore with `set_current_struct_tag(...)`.

## Suggested Next

First safe Step 3 code packet: tighten the
`parser_declarations.cpp` owner-scope save/restore paths by snapshotting both
`current_struct_tag_text_id` and fallback spelling, then restoring through a
small local rehydrate path instead of relying only on the saved raw string.
Keep `qualified_owner` spelling for emitted function names and
`owner_struct_tag` until a later packet audits those owner-spelling consumers.

## Watchouts

- Do not collapse `current_struct_tag` to `TextId` only yet:
  constructor/destructor matching still intentionally compares against
  `struct_source_name` for template-origin/injected spellings.
- `register_struct_member_typedef_binding(...)` remains spelling-keyed; changing
  it is a separate owner-registration packet, not the first safe cleanup.
- `parser_types_base.cpp` and `parser_expressions.cpp` currently use
  `current_struct_tag_text()` as a semantic gate/fallback accessor and do not
  need to be the first code packet.

## Proof

No build required for this inventory-only packet.

Local validation passed:
`git diff --check`

Proof log path: none updated; `test_after.log` was not touched.
