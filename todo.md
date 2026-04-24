Status: Active
Source Idea Path: ideas/open/86_parser_alias_template_structured_identity.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Tighten `last_using_alias_name` Around Structured Alias Identity

# Current Packet

## Just Finished

Step 4 inventory-only packet completed for `last_using_alias_name` active
context handoff sites.

Classification:

- `parser.hpp`: `clear_last_using_alias_name()` resets the structured alias key,
  fallback spelling, and `TextId` mirror together. `set_last_using_alias_name(
  const QualifiedNameKey&)` is the structured alias handoff path and preserves
  `base_text_id` for unqualified alias identity. `set_last_using_alias_name(
  std::string_view)` and `last_using_alias_name_text()` are fallback spelling /
  bridge compatibility helpers; no current parser call site uses them.
- `parser_declarations.cpp`: using-alias registration builds
  `alias_name_text_id` and `alias_key`, registers the structured typedef
  binding with that identity, and calls `set_last_using_alias_name(alias_key)`.
  This is structured alias lookup/handoff, with rendered `first_name` and
  `qualified` retained for compatibility typedef maps.
- `parser_declarations.cpp`: the template prelude clears alias handoff state,
  parses one top-level declaration, reads `last_using_alias_key`, and uses
  `find_structured_typedef_type(alias_key)` plus
  `template_state_.alias_template_info[alias_key]`. This is already structured
  alias-template metadata handoff.
- `parser_types_base.cpp`: no remaining `last_using_alias_name_text()`,
  `last_using_alias_name_text_id`, or `last_using_alias_key` consumers in the
  requested target file.
- `parser_types_template.cpp`: no remaining `last_using_alias_name_text()`,
  `last_using_alias_name_text_id`, or `last_using_alias_key` consumers in the
  requested target file.

## Suggested Next

Run one narrow Step 4 code packet in `parser.hpp`: remove the unused string-only
`set_last_using_alias_name(std::string_view)` fallback setter if the compiler
confirms no callers remain, and keep `last_using_alias_name_text()` only if the
next packet chooses to retain an explicit compatibility accessor for future
fallback spelling. If that removal is accepted, Step 4 can advance to Step 5;
the live declaration/template-prelude handoff already uses
`QualifiedNameKey`.

## Watchouts

- Do not remove rendered alias spelling passed to `register_typedef_binding(...)`
  in `parser_declarations.cpp`; that is compatibility bridge data for existing
  string-keyed maps, not the structured alias-template semantic key.
- `last_using_alias_name_text_id` is currently written by the structured setter
  but not read outside `last_using_alias_name_text()`. That makes the next code
  packet a helper cleanup, not a semantic lookup migration.
- `parser_types_base.cpp` and `parser_types_template.cpp` have no live Step 4
  handoff sites after the earlier template-struct cleanup.
- Do not rework alias-template storage beyond the Step 4 active-context
  handoff boundary.
- Do not reopen completed template-struct primary/specialization/instantiation
  lookup cleanup.

## Proof

No build required for this inventory-only packet.

Local validation: `git diff --check` passed.
Proof log: none updated; `test_after.log` was not touched.
