Status: Active
Source Idea Path: ideas/open/198_parser_legacy_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Parser Compatibility Routes

# Current Packet

## Just Finished

Step 1 - Inventory Parser Compatibility Routes completed.

Parser compatibility inventory:

- `resolve_record_type_spec_with_parser_tag_map_compatibility`
  (`src/frontend/parser/parser_support.hpp`,
  `src/frontend/parser/impl/support.cpp`): retained parser-local
  compatibility bridge. It first accepts direct `record_def`, then structured
  record context or unqualified `tag_text_id` by comparing candidate metadata,
  and only reaches rendered `TypeSpec::tag` for TextId-less legacy carriers.
  Classification: metadata-rich semantic lookup with an explicit legacy
  no-metadata tail. Step 2 should fence/convert callers that still pass complete
  structured metadata into this bridge instead of failing closed.
- `parser_record_layout_compatibility_tag_map`
  (`eval_const_int` in `parser_support.hpp` and
  `src/frontend/parser/impl/support.cpp`): retained record-layout parameter.
  `resolve_record_type_spec_for_constant_layout` accepts complete direct
  `record_def`, rejects structured misses, and allows parser-map lookup only
  for TextId-less legacy final-spelling carriers. Classification:
  no-metadata compatibility for constant layout, not ordinary semantic
  identity.
- `eval_const_int_with_rendered_named_const_compatibility`
  (`parser_support.hpp`, `src/frontend/parser/impl/support.cpp`): retained
  opt-in bridge for legacy/HIR template paths that carry rendered NTTP or
  named-constant names. Ordinary `eval_const_int` uses
  `std::unordered_map<TextId, long long>` via
  `Parser::eval_const_int_with_parser_tables`. Classification: explicit
  rendered no-metadata compatibility and later Step 4 target.
- `struct_tag_def_map`
  (`src/frontend/parser/impl/parser_state.hpp`): retained rendered-tag mirror.
  It is populated by parser record/template registration and still used by
  parser probes, template instantiation caches, static-member/base lookup, and
  testing helpers. Classification: parser-local scratch state plus
  compatibility mirror; semantic lookup should prefer `record_def`,
  structured keys, or Sema-owned lookup before this map.
- `defined_struct_tags`
  (`src/frontend/parser/impl/parser_state.hpp`,
  `src/frontend/parser/impl/types/struct.cpp`): retained rendered set used for
  parser definition/redefinition handling. It still gates a qualified-tag
  setup route that comments it lacks a structured `QualifiedNameRef`.
  Classification: parser-local scratch state and qualified-tag compatibility.
- Parser-local record setup
  (`parse_record_tag_setup`, `begin_record_body_context`,
  `register_record_definition` in
  `src/frontend/parser/impl/types/struct.cpp`): mostly structured
  `TextId`/context handoff, but qualified tag setup still falls back through
  rendered `defined_struct_tags`, and record registration writes rendered map
  mirrors under source and canonical spellings. Classification: parser-local
  scratch plus a narrow metadata gap for qualified record setup.
- Template `template_origin_name`
  (`src/frontend/parser/ast.hpp`,
  `src/frontend/parser/impl/types/template.cpp`,
  `src/frontend/parser/impl/types/base.cpp`,
  `src/frontend/parser/impl/expressions.cpp`): retained display/origin spelling
  used for instantiated records, mangled-name construction, and owner fallback.
  Existing code often builds structured `TemplateInstantiationKey` first, but
  `find_template_instantiated_record_by_structured_key` still re-interns
  candidate `template_origin_name`, and several static-member/base paths fall
  back to rendered map lookups when no structured carrier remains.
  Classification: source/display spelling plus compatibility authority when
  structured template identity is absent; Step 5 target.
- Rendered owner/tag recovery in parser expression construction
  (`src/frontend/parser/impl/expressions.cpp`): retained owner-name recovery
  uses `record_def`, `tag_text_id`, and then legacy display tag when building
  member-qualified expression refs. Classification: metadata-rich owner
  construction with stale-rendered fallback risk; Step 3 target.
- `src/shared/qualified_name_table.hpp` rendered helpers:
  `render_qualified_name`, `render_name_path`, and legacy split/base helpers
  are retained display/compatibility helpers. Comments state semantic authority
  must use `QualifiedNameKey`/`NamePathId` rather than a single rendered
  `TextId`. Classification: source/display/diagnostic spelling and legacy
  mirror inspection, not parser identity.
- Parser-focused existing proof candidates:
  `tests/frontend/frontend_parser_lookup_authority_tests.cpp` already has
  stale-rendered coverage around record constructor classification,
  `struct_tag_def_map` projection, visible owner lookup, template-origin drift,
  consteval TextId lookup, and rendered NTTP fallback suppression. These are
  evidence sources only for this packet; no expectations were changed.

First Step 2 target:

- Start with `resolve_record_type_spec_with_parser_tag_map_compatibility`
  callers in `src/frontend/parser/impl/types/template.cpp` and
  `src/frontend/parser/impl/types/base.cpp` that pass
  `definition_state_.struct_tag_def_map` after a `TypeSpec` already has
  structured carrier fields (`record_def`, `tag_text_id`,
  `namespace_context_id`, qualifier metadata, or `tpl_struct_origin_key`).
  This is the narrowest stale-rendered parser path because the bridge already
  fails closed for structured misses; Step 2 can prove/fence one remaining
  caller without touching named-const or template-origin policy.

## Suggested Next

Execute Step 2 from `plan.md`: fence one
`resolve_record_type_spec_with_parser_tag_map_compatibility` caller that has
complete structured `TypeSpec` metadata so it uses direct structured lookup or
fails closed instead of passing `struct_tag_def_map`.

## Watchouts

- Keep the work parser-owned; do not expand into Sema, HIR, LIR, BIR, or
  backend cleanup.
- Do not downgrade supported tests or replace semantic fixes with expectation
  rewrites.
- Do not treat parser diagnostics, AST display strings, source spelling, or
  final output text as semantic identity.
- Do not treat a raw `TextId` alone as complete semantic identity across
  scopes.
- Newly retained parser bridges need `legacy` or `deprecated` comments with
  owner, limitation, and removal condition.
- For Step 2, avoid the constant-layout
  `parser_record_layout_compatibility_tag_map` route at first unless the
  packet explicitly targets layout; it is already fenced to direct
  `record_def` plus TextId-less legacy carriers.
- Do not start with `eval_const_int_with_rendered_named_const_compatibility`;
  that route belongs to Step 4 and is a separate named-const surface.

## Proof

Inventory-only todo update; no implementation proof or `test_after.log` was
needed because no implementation files or tests were changed.
