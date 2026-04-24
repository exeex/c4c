Status: Active
Source Idea Path: ideas/open/86_parser_alias_template_structured_identity.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Active-Context Mirror Callers

# Current Packet

## Just Finished

Step 1 inventory completed for the active-context mirrors:
`last_using_alias_name`, `last_resolved_typedef`, and `current_struct_tag`.

Direct field/accessor inventory and classification:

- `parser.hpp` accessors:
  - `clear_current_struct_tag`, `set_current_struct_tag`,
    `current_struct_tag_text`: writer plus fallback spelling accessor for
    `current_struct_tag`/`current_struct_tag_text_id`.
  - `clear_last_resolved_typedef`, `set_last_resolved_typedef`,
    `find_current_typedef_fn_ptr_info`: writer plus structured `TextId` lookup
    for typedef function-pointer metadata.
  - `clear_last_using_alias_name`, `set_last_using_alias_name(QualifiedNameKey)`,
    `set_last_using_alias_name(string_view)`, `last_using_alias_name_text`:
    writer plus structured alias key/`TextId` fallback bridge.
- `parser_support.cpp` save/restore:
  - `save_lite_state` stores only `last_resolved_typedef_text_id` when valid.
  - `restore_lite_state` clears typedef state and reconstructs spelling through
    `set_last_resolved_typedef(token_texts->lookup(text_id))`.
  - `TentativeParseGuard` and `TentativeParseGuardLite` restore through these
    snapshots, so typedef rollback is already `TextId`-first but still
    rehydrates the string mirror.
- `last_resolved_typedef` direct uses:
  - `parser_expressions.cpp` sets it for direct typedef cast parsing, manually
    saves/restores both string and `TextId` around a qualified-type tentative
    parse, and uses `find_current_typedef_fn_ptr_info()` for function-pointer
    cast node metadata. Classification: structured lookup plus
    snapshot/rollback.
  - `parser_types_base.cpp` sets it when a typedef type is resolved, then uses
    `last_resolved_typedef_text_id` for alias-template lookup and direct
    template-primary probing with string fallback spelling. Classification:
    structured lookup with fallback spelling.
  - `parser_declarations.cpp` reads only `last_resolved_typedef_text_id` into
    `ret_typedef_name_id` for return-type function-pointer propagation.
    Classification: structured lookup / metadata handoff.
- `current_struct_tag` direct uses:
  - `parser_declarations.cpp` incomplete-object checks compare
    `current_struct_tag_text_id` plus `current_struct_tag_text()` against the
    candidate tag, while the string field is used as an emptiness gate.
    Classification: structured lookup with fallback spelling gate.
  - `parser_expressions.cpp` operator-call owner collapse gates on the string
    field and compares `current_struct_tag_text()` against visible owner names.
    Classification: structured lookup / member-call disambiguation.
  - `parser_types_base.cpp` declaration/type probes use
    `current_struct_tag_text()` for template-type fallback and member function
    name construction. Classification: fallback spelling and structured lookup
    escape hatch.
  - `parser_types_struct.cpp` member alias/typedef registration uses
    `current_struct_tag_text()` as the owner spelling; constructor/destructor
    parsing gates on the string field, and special-member matching compares
    against `current_struct_tag_text()`. Classification: fallback/injected
    spelling plus parser context gate.
  - Record-body and out-of-class member paths save the string mirror, call
    `set_current_struct_tag(...)`, and restore by spelling:
    `begin_record_body_context`/`finish_record_body_context`,
    `parse_record_body_with_context`, qualified operator/constructor parsing,
    and qualified-owner top-level parsing. Classification: snapshot/rollback
    plus injected/emitted/member-owner spelling.
- `last_using_alias_name` direct uses:
  - `parser_declarations.cpp` sets a `QualifiedNameKey` after registering a
    using-alias typedef; template declaration parsing clears before
    `parse_top_level`, reads `last_using_alias_key`, records
    `alias_template_info[alias_key]`, then clears again. Classification:
    structured alias lookup/handoff.
  - No current parser call site uses `last_using_alias_name_text()` outside the
    accessor; string spelling is fallback bridge state only.

## Suggested Next

Execute Step 2 with a narrow `last_resolved_typedef` packet: replace the manual
qualified-type tentative save/restore in `parser_expressions.cpp` with a
small helper or local restore path that treats
`last_resolved_typedef_text_id` as authoritative and rehydrates spelling only
through the existing accessor. Keep `parser_types_base.cpp` alias-template and
template-primary probes unchanged except for any call-site cleanup needed by
the helper.

## Watchouts

- The first code packet should not change alias-template storage or
  template-struct primary/specialization logic; those paths are already using
  structured keys with fallback probes.
- `last_resolved_typedef` is the lowest-risk first family because its global
  tentative snapshot is already `TextId`-first. The main remaining string
  dependency is the manual save/restore around qualified-type expression
  parsing.
- `current_struct_tag` has more spelling-sensitive owner paths; defer it until
  typedef rollback cleanup is proven.
- `last_using_alias_name` currently has no direct text-reader outside the
  accessor, so it is better as a later audit/cleanup packet unless new alias
  handoff uses appear.

## Proof

Inventory-only packet; no build required by supervisor. Local proof passed:
`git diff --check`.
