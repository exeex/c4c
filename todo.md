Status: Active
Source Idea Path: ideas/open/145_move_record_tag_authority_from_parser_to_sema.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Current Authority

# Current Packet

## Just Finished

Completed Step 1: Inventory Current Authority. Current record/tag authority
surfaces:

- Parser `ParserDefinitionState::defined_struct_tags`: compatibility/testing
  support. It is a rendered-tag set exposed through `has_defined_struct_tag`
  and tests, but no longer looks like the preferred semantic table.
- Parser `ParserDefinitionState::struct_tag_def_map`: compatibility plus
  remaining semantic authority. It is documented as a rendered-tag mirror, but
  still backs parser record layout fallbacks, tag redefinition/forward lookup,
  template instantiation record recovery, and several parser support callers.
- Parser `TypeSpec::record_def`: provisional carrier production today, with
  parser-owned semantic authority risk. It carries concrete `NK_STRUCT_DEF`
  identity into support, typedef, template, declaration, and expression paths.
- Parser `TypeSpec::tag_text_id`, `namespace_context_id`,
  `qualifier_text_ids`, `is_global_qualified`: provisional carrier production.
  These are spelling/context metadata only and must not become semantic record
  identity by themselves.
- Parser `Node` record name fields and qualifier metadata
  (`name`, `unqualified_name`, `unqualified_text_id`, qualifier fields):
  provisional carrier production plus diagnostics/display. Sema and HIR derive
  structured keys from them, but final display/rendered names remain separate.
- Parser `parse_record_tag_setup`: semantic authority and compatibility. It
  uses `struct_tag_def_map` to reuse an existing completed record for rendered
  qualified tags before Sema can decide declaration/definition merging.
- Parser `register_record_definition`: compatibility plus provisional carrier
  production. It writes rendered source/canonical keys to `struct_tag_def_map`
  and registers typedef/tag bindings that carry `record_def`, TextIds, and
  namespace/qualifier metadata.
- Parser `resolve_record_type_spec`: compatibility plus remaining semantic
  authority. It prefers `TypeSpec::record_def`, but still scans or indexes
  `struct_tag_def_map` by `tag_text_id` or rendered spelling.
- Parser layout helpers `struct_sizeof`, `struct_align`, `field_align`, and
  `compute_offsetof`: semantic authority for parser constant layout queries
  because they call `resolve_record_type_spec` and can fall back to the
  rendered-tag map.
- Parser `eval_const_int` overloads: semantic authority for `sizeof(type)`,
  `_Alignof(type)`, and `__builtin_offsetof(type, member)` during parser-side
  constant folding; the string-map overload is an explicit compatibility
  bridge, while the TextId const-binding overload still accepts the record map.
- Parser grammar/type disambiguation surfaces in `parse_base_type`,
  typedef/tag binding tables, visible typedef scopes, and record member/base
  dispatch: grammar disambiguation plus provisional carrier production. These
  may stay parser-owned when they only choose parse branches or carry source
  metadata.
- Parser diagnostics/display helpers, AST dumps, legacy display tag helpers,
  and final rendered names: diagnostics/display or compatibility only.
- Parser/frontend metadata tests under `tests/frontend` and parser lookup
  authority tests: testing support. They currently prove that `record_def` or
  TextId metadata beats stale rendered tags, but do not yet prove Sema-owned
  record-domain identity.
- Sema `SemaStructuredNameKey`: semantic authority candidate. It combines
  namespace context, global qualification, qualifier TextIds, and base TextId,
  but is a general structured-name key, not yet a record-domain-specific key.
- Sema `struct_defs_by_key_`, `complete_structs_by_key_`,
  `complete_unions_by_key_`: semantic authority. These are the strongest
  current Sema-owned record mirrors and should become the destination for
  declaration/reference/completion decisions.
- Sema `structured_record_keys_by_tag_` and
  `ambiguous_structured_record_tags_`: compatibility. They bridge rendered tags
  back to structured keys only when unambiguous; they should not be expanded as
  primary authority.
- Sema `complete_structs_` and `complete_unions_`: compatibility. These are
  rendered-string completion sets used beside the structured-key sets.
- Sema `struct_defs_by_unqualified_name_`: compatibility/diagnostics support.
  It is useful for unqualified-name fallback and display-oriented lookup, not
  final record identity.
- Sema `structured_record_key_for_type`,
  `structured_record_key_for_type_metadata`, and `is_complete_object_type`:
  semantic authority plus compatibility. They prefer record/key metadata but
  still compare or fall back to rendered-tag completion.
- Sema record member/static-member maps
  (`struct_field_text_ids_by_key_`, `struct_static_member_types_by_key_`,
  `struct_base_keys_by_key_`) and method-owner lookup by structured key:
  semantic authority for member availability after a record key is known.
- Sema type identity helpers in `type_utils.cpp`: semantic authority for type
  equality. They prefer `record_def`, then complete TextId/context metadata,
  then compatibility rendered-name comparison when no structured metadata
  exists.
- Sema consteval record owner/layout lookup: semantic authority plus
  compatibility. It derives HIR record owner keys from `record_def` or
  TypeSpec metadata, then uses rendered-tag canonicalization only as a
  temporary link-name TextId bridge.
- Sema diagnostics for incomplete `sizeof`/`alignof`: diagnostics plus
  semantic authority. They currently rely on `is_complete_object_type`.

First lookup family to migrate: parser constant-layout record lookup for
`sizeof(type)` / `_Alignof(type)` / `__builtin_offsetof(type, member)`, starting
at `resolve_record_type_spec` and its `struct_sizeof`, `struct_align`, and
`compute_offsetof` callers. This is the narrowest high-value family because it
directly consumes `struct_tag_def_map` as a fallback semantic record layout
table while Sema already has structured completion/key mirrors for the same
record-completeness domain.

## Suggested Next

Delegate Step 2 to define the parser provisional carrier contract for the
constant-layout record lookup family, keeping `record_def`/TextId/context
metadata as source carriers and routing final record identity toward the Sema
record-key table.

## Watchouts

Do not migrate by making `tag_text_id` alone authoritative. The current Sema
key is structured but still general-purpose; the next slice should either
bound it as an interim record-domain key or make the missing record-domain
contract explicit before replacing parser fallback lookups.

## Proof

Inventory-only lifecycle/documentation packet. No implementation files changed;
no build/test required by the delegated proof, and no `test_after.log` was
produced.
