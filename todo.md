Status: Active
Source Idea Path: ideas/open/193_hir_constructor_member_owner_structured_lookup_closure.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Record Remaining Compatibility Surface

# Current Packet

## Just Finished

Step 4 - Record Remaining Compatibility Surface completed after the direct
struct constructor structured-owner conversion and proof.

Remaining rendered owner surface ledger:

- Direct struct constructor calls: selected route is covered. `callee_name`
  remains final spelling and no-owner compatibility input, but complete
  `record_def` or tag/namespace owner metadata now routes through
  `resolve_lowerer_registry_struct_tag`; complete structured misses return no
  constructor expression instead of recovering through stale rendered spelling.
- Direct constructor `struct_def_nodes_.find(callee_name)`: retained as
  simple rendered-name recovery when the AST/type node did not carry richer
  owner metadata. Owner: direct constructor owner seed. Reason: compatibility
  for simple unqualified constructor references. Removal condition: constructor
  refs always carry complete record owner metadata or a structured owner key
  before HIR lowering.
- `resolve_lowerer_registry_struct_tag` rendered fallback: retained as the
  explicit no-owner/no-metadata compatibility boundary. Owner:
  registry-backed constructor/new-expression owner lookup. Reason: callers may
  still arrive with only rendered final spelling. Removal condition: all
  registry-lookup callers carry complete `record_def`, tag/namespace owner
  metadata, or template owner metadata.
- `find_struct_def_by_no_owner_layout_compatibility_tag` and
  `rendered_typespec_tag_for_compatibility`: retained for layout/type
  compatibility when the `TypeSpec` lacks complete owner metadata. Owner:
  layout lookup helper. Reason: downstream layout consumers still use rendered
  `Module::struct_defs` tags as the storage handle. Removal condition:
  `TypeSpec` layout lookup and `Module::struct_defs` consumers move to
  structured record-owner keys or a structured layout handle.
- Initializer-list layout scan over `module_->struct_defs`: retained as
  route-local layout matching by field shape/element type, not stale owner
  recovery. Owner: initializer-list materialization. Reason: no single owner is
  available when matching an initializer-list wrapper layout. Removal
  condition: initializer-list construction carries a structured wrapper owner
  from parser/Sema or a typed library layout handle.
- Initializer-list member assignment owner lookup: retained through
  `resolve_member_lookup_owner_tag` and selected-layout fallback. Owner:
  `_M_array` / `_M_len` member lowering. Reason: member symbol lookup needs a
  rendered layout tag when complete member owner metadata is absent. Removal
  condition: initializer-list temporary `TypeSpec` always carries complete
  owner/member metadata and member symbol ids are resolved solely through
  structured owner/member keys.
- Compound-literal aggregate owner lookup: retained with structured-first
  fencing. Record/template owners use structured lookup or materialization;
  `legacy_tag` lookup is used only when `has_structured_owner` is false. Owner:
  compound-literal aggregate/member lowering. Reason: aggregate values may
  still be lowered from legacy-only `TypeSpec` carriers. Removal condition:
  aggregate `TypeSpec` producers always preserve complete owner keys; a later
  cleanup should also consider whether `has_aggregate_structured_owner_identity`
  should use the same complete tag/namespace predicate as direct constructors.
- Base-layout lookup through `base_tags`: retained as explicit legacy base
  metadata compatibility after `base_tag_text_ids` fail to form a complete
  owner key. Owner: inherited field/layout lookup. Reason: base layout metadata
  still stores rendered final tags. Removal condition: base records store and
  consume complete structured owner keys for all inherited-layout lookup.
- `resolve_member_lookup_owner_tag` final
  `legacy_owner_tag_from_type_if_no_metadata` path: retained as explicit
  member-owner compatibility. Owner: member/method/aggregate helper callers.
  Reason: some callers still provide only legacy `TypeSpec::tag`. Removal
  condition: all member-lookup callers provide complete record owner metadata,
  template owner metadata, or an already resolved layout owner.
- `resolve_struct_method_lookup_owner_tag` tag-text/legacy fallback and
  `find_struct_method_*` rendered maps: retained for class-specific
  new/delete/operator method compatibility. Owner: method lookup helpers.
  Reason: complete owner-key hits already use owner-indexed maps and rendered
  fallback is suppressed after complete owner-key misses, but no-owner callers
  still depend on rendered method keys. Removal condition: class-specific
  method lookup callers and registries are fully keyed by structured owner
  identity plus method `TextId`.

No separate follow-up idea is required for the selected direct-constructor
closure route. The remaining surfaces above are either explicit
no-owner/no-metadata compatibility or broader object/member lowering cleanup;
if the supervisor wants to delete those compatibility paths, that should be a
new follow-up idea because it exceeds this narrow selected-route acceptance
slice.

## Suggested Next

Run Step 5 validation/closure evidence for the selected direct-constructor
route, including the focused stale rendered fallback proof and any
milestone-appropriate broader validation the supervisor selects.

## Watchouts

This packet is documentation-only and did not change code, tests, plans, ideas,
or logs. The ledger intentionally does not claim removal of all rendered
`Module::struct_defs` handles; it records which remaining uses are
compatibility/storage handles after the direct-constructor semantic route was
converted.

## Proof

Documentation/todo-only packet. No validation was run because no code changed.
No `test_after.log` was produced or modified for this packet.
