Status: Active
Source Idea Path: ideas/open/193_hir_constructor_member_owner_structured_lookup_closure.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Constructor And Member Owner Authority

# Current Packet

## Just Finished

Step 1 - Inventory Constructor And Member Owner Authority completed.

Owner lookup ledger for `src/frontend/hir/impl/expr/object.cpp` and nearby helper
APIs:

- `object.cpp:20` `set_typespec_final_spelling_tag_if_present`: display/final
  spelling mirror. It writes legacy `TypeSpec::tag` when that field exists; it
  should not be semantic owner authority by itself.
- `object.cpp:28` `typespec_legacy_tag_if_present`: no-owner compatibility
  reader. Current uses are acceptable only after structured metadata has been
  ruled out.
- `object.cpp:37` `typespec_has_aggregate_nominal_carrier` and `object.cpp:43`
  `typespec_has_aggregate_structured_identity_metadata`: route-local aggregate
  gates. They distinguish "some nominal spelling exists" from metadata-rich
  aggregate identity; acceptable as classification helpers.
- `object.cpp:52` `aggregate_direct_assign_owner_key`: structured-authority
  builder for aggregate direct assignment. It forms `HirRecordOwnerKey` from
  `record_def` or complete `tag_text_id`/namespace metadata.
- `object.cpp:77` `same_aggregate_direct_assign_type`: structured-authority
  mirror. If either side has structured aggregate metadata, both sides must
  produce matching owner keys and resolve through
  `module->find_struct_def_tag_by_owner`; legacy tag comparison is reserved for
  no-carrier compatibility.
- `object.cpp:148` `callee_name` in
  `try_lower_direct_struct_constructor_call`: rendered spelling source. It is
  used for final spelling and compatibility, but can still become authority
  when the route does not recognize complete tag/namespace metadata.
- `object.cpp:152` `has_structured_owner` in direct constructor lowering:
  metadata-rich authority bug candidate. It treats `record_def`,
  `tpl_struct_origin`, and explicit template args as structured ownership, but
  misses complete `tag_text_id` plus namespace metadata. That can send a
  metadata-rich owner to `lookup_callee_tag = callee_name`.
- `object.cpp:166` `struct_def_nodes_.find(callee_name)`: rendered-name
  recovery for direct constructors. This can seed `record_def` for simple
  cases, but it is not structured authority and should not be the route's
  answer when complete owner metadata is already present.
- `object.cpp:200` `resolve_lowerer_registry_struct_tag(...,
  "direct-ctor-owner", &callee_name)`: selected structured route helper. Nearby
  implementation in `hir_types.cpp:803` fail-closes complete `record_def` and
  complete tag/namespace owner-key misses before reaching rendered compatibility;
  its final `rendered_compat_tag` fallback is explicit no-metadata
  compatibility.
- `object.cpp:211`, `object.cpp:240`, and `object.cpp:241`
  `module_->struct_defs` / `struct_constructors_` lookups by
  `lookup_callee_tag`: route-local handles after owner resolution. These are
  acceptable if `lookup_callee_tag` came from structured authority or explicit
  no-owner compatibility.
- `object.cpp:302` deleted-constructor diagnostic uses `lookup_callee_tag`:
  display/diagnostic only.
- `object.cpp:307` `ensure_constructor_overload_lowered(...,
  *lookup_callee_tag, ...)`: route-local lowering handle, not an independent
  owner decision.
- `object.cpp:410` `materialize_initializer_list_arg`
  `apply_initializer_list_layout_owner`: structured-authority mirror when a
  selected `HirStructDef` layout is already known; it copies tag text and
  namespace metadata onto `list_ts`.
- `object.cpp:467` initializer-list layout scan over `module_->struct_defs`:
  route-local layout search by initializer-list element shape. It is not owner
  recovery from rendered callee/tag spelling, but it remains a broad
  compatibility surface.
- `object.cpp:509` instantiated initializer-list layout tag from
  `build_template_mangled_name`: template materialization handle. Acceptable
  when derived from template args, not a stale rendered fallback.
- `object.cpp:552` `find_struct_def_for_layout_type(list_ts)`:
  structured-authority mirror when type metadata resolves layout; any no-owner
  layout compatibility is inside the helper.
- `object.cpp:628` initializer-list member assignment
  `resolve_member_lookup_owner_tag`: structured/member helper. It first attempts
  structured owner resolution and only reaches `legacy_owner_tag_from_type_if_no_metadata`
  in `value_args.cpp:640`, so rendered fallback is fenced as no-metadata
  compatibility.
- `object.cpp:637` and `object.cpp:642` initializer-list member owner fallbacks
  through `find_struct_def_for_layout_type` or selected layout: route-local
  layout mirrors for `_M_array` / `_M_len`; acceptable because member symbol
  lookup below uses `TypeSpec` metadata when available.
- `object.cpp:635`, `object.cpp:639`, and `object.cpp:643`
  `find_struct_member_symbol_id(list_ts, rendered_tag, field_name, text_id)`:
  structured-member authority mirror. `hir_types.cpp:2425` forms a structured
  member key from `TypeSpec`; stale rendered member ids only win when no
  complete key can be formed.
- `object.cpp:782` `has_aggregate_structured_owner_identity`:
  aggregate route gate. It uses `record_def`/template carriers but not complete
  `tag_text_id`/namespace metadata; lower risk than direct constructors because
  `resolve_aggregate_owner` has separate no-structured fencing, but worth
  revisiting after the selected route.
- `object.cpp:793` `resolve_structured_aggregate_owner_tag`: structured
  aggregate authority for compound literal members. Record defs use
  `find_struct_def_tag_by_owner`; template owners are resolved/materialized and
  require a changed/resolved tag present in `module_->struct_defs`.
- `object.cpp:838` `resolve_aggregate_owner`: fenced route. Structured owners
  go through `resolve_structured_aggregate_owner_tag`; `resolve_member_lookup_owner_tag`
  and legacy tag lookup are only used when `has_structured_owner` is false.
- `object.cpp:872` `resolve_aggregate_member_symbol`: member symbol route-local
  mirror from resolved aggregate owner. It falls back to the field's already
  recorded member symbol id if the owner-table mirror is missing.
- `object.cpp:1066` new-expression `resolve_struct_method_lookup_owner_tag`:
  class-specific allocation method lookup. `hir_types.cpp:787` may return
  rendered tag text after `resolve_member_lookup_owner_tag` misses, so this is
  explicit compatibility for method lookup, outside the constructor/member
  route selected here.
- `object.cpp:1082` new-expression constructor owner
  `resolve_lowerer_registry_struct_tag`: structured constructor owner helper
  already has fail-closed complete metadata behavior in `hir_types.cpp:803`.
- `object.cpp:1120` new-expression `struct_constructors_` lookup by
  `alloc_ctor_owner_tag`: route-local handle after helper resolution.
- `object.cpp:1185` delete-expression `resolve_struct_method_lookup_owner_tag`:
  class-specific delete method lookup; same compatibility classification as
  new-expression allocation method lookup.
- `hir_types.cpp:803` `resolve_lowerer_registry_struct_tag`: metadata-rich
  route authority. Complete `record_def` and complete tag/namespace owner keys
  resolve via `module_->find_struct_def_tag_by_owner` and return `nullopt` on a
  complete miss; rendered fallback is only after structured/template metadata
  has not produced a complete owner.
- `value_args.cpp:486` `resolve_member_lookup_owner_tag`: member owner helper.
  It prioritizes current injected-class, record owner, structured type metadata,
  and template materialization before `legacy_owner_tag_from_type_if_no_metadata`.
- `hir_types.cpp:2340` and `hir_types.cpp:2425`
  `find_struct_member_symbol_id`: member-symbol authority helpers. Complete
  structured owner/member keys use `struct_member_symbol_ids_by_owner_` or
  structured field ids and do not let rendered ids win; rendered table lookup is
  explicitly documented as no-owner compatibility.

Selected Step 2 route: direct struct constructor lowering in
`try_lower_direct_struct_constructor_call`. Rationale: this is a narrow
metadata-rich constructor path matching idea 193 acceptance criteria, and the
available `resolve_lowerer_registry_struct_tag` helper already provides the
desired behavior for complete structured owner misses. Step 2 should expand the
direct-ctor `has_structured_owner` gate to include complete `tag_text_id` plus
namespace/qualifier owner metadata, then call the registry helper for that
metadata-rich path and keep `callee_name` only as the explicit no-owner
compatibility fallback. That should make stale rendered constructor tags fail
closed without rewriting aggregate/member lowering broadly.

## Suggested Next

Execute Step 2 on the direct constructor route: make metadata-rich direct
constructor owner lookup recognize complete `tag_text_id`/namespace metadata
and route it through `resolve_lowerer_registry_struct_tag`, with `callee_name`
reserved for explicit no-owner compatibility.

## Watchouts

Keep direct-ctor diagnostics/display spelling intact. Do not change
`struct_constructors_` storage or overload policy in Step 2; the selected slice
only decides how `lookup_callee_tag` is produced. The similar aggregate gate at
`object.cpp:782` does not count tag/namespace metadata either, but it should be
left as a later surface unless Step 2 proves the helper needs a shared predicate.
Idea 195 remains blocked by open idea 193 and still has stale dependency paths
involving closed ideas; repair those paths only when activating or closing that
gate requires it.

## Proof

Inventory-only packet; no build or test validation was run because no code
changed. No `test_after.log` was produced for this non-code proof.
