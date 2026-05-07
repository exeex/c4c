# Current Packet

Status: Active
Source Idea Path: ideas/open/149_template_instantiation_structured_argument_key.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Template Argument String Authority

## Just Finished

Completed Step 1 inventory for template-instantiation argument string
authority. Reachable semantic string-authority sites:

- `src/frontend/parser/impl/parser_state.hpp`: `TemplateInstantiationKey::Argument`
  owns `bool is_value` plus `std::string canonical_key`; equality compares the
  string, `TemplateInstantiationKeyHash` hashes it, and both
  `instantiated_template_struct_keys_by_key` and
  `template_instantiation_member_typedefs_by_key` consume it as semantic key
  material.
- `src/frontend/parser/impl/types/types_helpers.hpp`:
  `make_template_instantiation_argument_key` writes `canonical_key` for every
  parsed argument. Type arguments route through
  `canonical_template_struct_type_key`; value arguments route through
  `canonical_template_nttp_expr_key`, `$tokens:<kind>#<text_id>;...`,
  legacy `$expr:` text, or `std::to_string(value)`.
- `src/frontend/parser/impl/types/template.cpp`:
  `ensure_template_struct_instantiated_from_args` builds a
  `TemplateInstantiationKey` with `make_template_instantiation_argument_keys`
  and records it through `mark_template_instantiation_dedup_keys`, so struct
  instantiation dedup still depends on `canonical_key`.
- `src/frontend/parser/impl/types/base.cpp`: direct-emission dedup helpers
  `has_template_instantiation_dedup_key_for_direct_emit` and
  `mark_template_instantiation_dedup_key_for_direct_emit` consume the same key;
  member-typedef lookup families build concrete owners from
  `make_template_instantiation_argument_keys` at substituted record-owner,
  direct emitted owner, alias-owner, alias member-typedef, and emitted-instance
  registration sites.
- `src/frontend/parser/impl/core.cpp`: 
  `register_template_instantiation_member_typedef_binding` stores concrete
  owner keys and `find_template_instantiation_member_typedef_type` performs
  lookup through the `TemplateInstantiationMemberTypedefKey` hash/equality
  path, making argument-string equality observable.

Display-only or exclusion sites:

- `src/frontend/sema/canonical_symbol.hpp` comments and
  `format_template_arg` declarations are Sema debug/display formatting, not
  the parser `TemplateInstantiationKey` authority.
- `src/frontend/hir/hir_ir.hpp` `SpecializationKey::canonical` is a separate
  HIR function-template specialization serialization path, not a consumer of
  parser `TemplateInstantiationKey::Argument::canonical_key`.
- `TemplateArgRef::debug_text`, `rendered_template_arg_refs_from_parsed_args`,
  and `set_template_arg_debug_refs_text` are display/compatibility mirrors
  unless fed back through `canonical_template_struct_type_key` fallback.
- Tests that hand-fill `Argument::canonical_key` or assert the old canonical
  string are probes/fixtures, not production semantic authority, but they will
  need adjustment after the representation changes.

Type-argument caller family:

- Parser type parsing produces `TemplateArgParseResult{is_value=false,
  type=TypeSpec}` in `try_parse_template_type_arg`,
  `parse_template_argument_list`, qualified template-id parsing, alias member
  typedef carriers, and record specialization parsing.
- Type arguments enter semantic instantiation keys through
  `make_template_instantiation_argument_key -> canonical_template_struct_type_key`.
  Nested `TemplateArgRef` type arguments are recursively string-encoded; the
  current fallback uses `debug_text` or `t:?` when no structured type key is
  available.

Value-argument caller family:

- Parser NTTP parsing produces `TemplateArgParseResult{is_value=true}` from
  literals, parsed expression nodes, captured token carriers, forwarded NTTP
  names/text IDs, default NTTP values, and alias/record substitution carriers.
- Value arguments enter semantic instantiation keys through
  `canonical_template_nttp_expr_key`, captured token string encoding, legacy
  `$expr:` text, or numeric `value`; nested `TemplateArgRef` values in
  `canonical_template_struct_type_key` currently collapse to `v:<value>`.

## Suggested Next

First narrow implementation packet: change only the core representation and
helper boundary by replacing `TemplateInstantiationKey::Argument::canonical_key`
with an explicit structured variant in `parser_state.hpp`, then update
`make_template_instantiation_argument_key` in `types_helpers.hpp` to populate
type, value-expression, token-carrier, legacy-expression-compat, and numeric
value cases without moving parser call sites yet.

## Watchouts

- Do not treat raw `TextId` spelling as the full semantic template argument key.
- Do not weaken tests or mark supported cases unsupported as a shortcut around
  structured argument identity.
- Keep rendered argument text display-only or compatibility-only unless the
  source idea is explicitly revised.
- The `canonical_template_struct_type_key` fallback to `TemplateArgRef::debug_text`
  is the main type-side string fallback to isolate; avoid just renaming it into
  the new argument variant.
- Captured NTTP tokens are structured enough to carry token kind and `TextId`,
  but the current `$tokens:` concatenation is still a string-shaped semantic
  key and should become a real token-vector payload or explicit compatibility
  variant.
- HIR has separate template-instantiation machinery over `TemplateArgRef` and
  HIR specialization keys; do not widen the first packet into HIR unless the
  parser key representation needs a carrier consumed there.

## Proof

Inventory-only lifecycle scratchpad update. No build or test command was run,
and no `test_after.log` was produced because the delegated packet explicitly
required no proof command unless implementation files changed.
