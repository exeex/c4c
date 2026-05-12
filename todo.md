Status: Active
Source Idea Path: ideas/open/199_sema_legacy_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Sema Compatibility Routes

# Current Packet

## Just Finished

Step 1 inventoried the active Sema-owned rendered compatibility routes and
classified the first conversion/fencing target.

- Static-eval enum compatibility:
  `StaticEvalIntEnumLookupInput::with_rendered_enum_compatibility`,
  `static_eval_int_with_rendered_enum_compatibility`, and the HIR static-member
  enum initializer callers retain rendered enum maps as legacy/no-metadata
  compatibility. `enum_consts_by_key` and `enum_consts_by_text` are the
  metadata-rich semantic lookup authority, and complete misses already return
  zero before rendered enum lookup.
- Sema type identity:
  `same_rendered_type_name_compatibility` is legacy/no-metadata compatibility
  only. It is reached after template-param, record-def, complete TextId, and
  partial TextId metadata comparisons reject rendered spelling as authority.
- Consteval type-binding bridges:
  `type_bindings` is the rendered no-metadata substitution mirror.
  `type_bindings_by_text` and `type_bindings_by_key` are metadata-rich semantic
  lookup authority. `type_binding_text_ids_by_name` and
  `type_binding_keys_by_name` are legacy display-tag bridge indexes that select
  TextId/key authority but are not binding authority themselves.
- Consteval NTTP bridges:
  `nttp_bindings` is the rendered no-metadata output mirror for legacy
  forwarded parameter spellings. `nttp_bindings_by_text` and
  `nttp_bindings_by_key` are metadata-rich semantic lookup authority, while
  literal `template_arg_values` are source payload rather than rendered-name
  lookup.
- Rendered consteval function lookup:
  `consteval_fns` is the legacy/no-metadata recursive or chained call map.
  `consteval_fns_by_text` and `consteval_fns_by_key` are authority for
  metadata-rich call sites, and structured/key misses should fail closed before
  rendered lookup.
- Interpreter and validate local const mirrors:
  interpreter `by_name`, `local_consts`, `local_const_scopes`, and HIR
  `rendered_compat_local_names` are rendered legacy/no-metadata mirrors.
  `by_text`, `by_key`, `local_consts_by_text`, `local_consts_by_key`, and
  scoped TextId/key maps are metadata-rich semantic lookup authority.
- `ConstEvalEnv::struct_defs` layout handoff:
  `struct_defs` is rendered legacy layout compatibility for late HIR layout
  handoff. `struct_def_owner_index` plus `record_owner_key_from_typespec` is
  the semantic authority. `link_name_texts` is a bridge that canonicalizes a
  rendered spelling back to TextId/key lookup, not final authority.
- Separate HIR follow-up scope:
  broad HIR lowerer rendered maps and local-name sets that do not sit at the
  Sema/consteval boundary belong to the later HIR compatibility track, except
  where a Sema proof needs the boundary caller.

## Suggested Next

Begin Step 2 by fencing the static-eval enum compatibility route. The first
narrow target should be `StaticEvalIntEnumLookupInput` / `static_eval_int`
coverage: add or tighten stale-rendered enum proof showing that a complete
structured enum key or TextId miss does not recover through
`rendered_enum_consts`, then either delete impossible rendered recovery or mark
the retained rendered enum map as explicit legacy/no-metadata compatibility
with owner, limitation, and removal condition.

## Watchouts

- Keep the work Sema-owned; do not expand into parser, broad HIR lowerer, BIR,
  LIR, backend, or full consteval-engine rewrites.
- Do not claim progress through helper renames, diagnostics-only changes, or
  expectation rewrites.
- Covered metadata-rich Sema misses must not recover through rendered enum,
  consteval function, NTTP, type-binding, local-const, or struct-def maps.
- Retained Sema bridges need `legacy` or `deprecated` comments with owner,
  limitation, and removal condition.
- Several routes already have fail-closed behavior and comments. Step 2 should
  still prove the enum route with stale rendered input instead of treating this
  inventory as implementation progress.

## Proof

Inventory-only `todo.md` update. No code validation required by the delegated
packet; no `test_after.log` was produced.
