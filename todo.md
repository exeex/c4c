# Current Packet

Status: Active
Source Idea Path: ideas/open/172_type_identity_authority_audit.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Inventory HIR Type And Record Authority

## Just Finished

Step 2 - Inventory HIR Type And Record Authority completed as a code-reading
audit.

Files inspected:
- `src/frontend/hir/hir.hpp`: public HIR facade and `ResolvedTypeTable`
  handoff into `build_hir`.
- `src/frontend/hir/hir_ir.hpp`: HIR type carriers, declaration lookup keys,
  template parameter binding keys, specialization keys, record owner keys,
  struct/member/method lookup keys, module indexes, and rendered compatibility
  lookup bridges.
- `src/frontend/hir/compile_time_engine.hpp`: template seed/instance keys,
  `PendingTemplateTypeKey`, pending work queues, structured vs legacy
  specialization construction, and display-only pending-key rendering.
- `src/frontend/hir/hir_build.cpp`: AST-to-HIR collection, template seed
  recording, struct owner collection, out-of-class method lookup, ref-overload
  owner matching, deferred template type instantiation, and initial lowering
  orchestration.
- `src/frontend/hir/hir_types.cpp`: typedef-to-record resolution, HIR
  `TypeSpec`/`QualType` construction, layout lookup helpers, struct/member
  owner lookup registration and query paths, static member lookup, field
  lowering, record layout population, and method pending work.
- `src/frontend/hir/hir_lowering_core.cpp`: `compute_struct_layout` and the
  final layout computation handoff for `HirStructDef`.
- `src/frontend/hir/impl/lowerer.hpp`: `Lowerer` state and map inventory for
  local/function type refs, pending methods, struct owner mirrors, template
  bindings, and deferred template type helpers.
- `src/frontend/hir/impl/templates/templates.cpp`: template struct primary
  registration, owner-aware primary lookup, pending-template seeding, and
  rendered-origin recovery.
- `src/frontend/hir/impl/templates/member_typedef.cpp`: deferred member
  typedef owner resolution and structured fail-closed behavior.
- `src/frontend/hir/impl/templates/struct_instantiation.cpp`: template struct
  instance owner-key creation, instantiated field/method/static-member
  registration, layout computation, and instance dedup.
- `src/frontend/hir/impl/templates/value_args.cpp` and
  `src/frontend/hir/impl/templates/deferred_nttp.cpp` were searched for
  owner-key static-member/value routes and rendered compatibility call sites.
- `src/frontend/sema/consteval.hpp` and `src/frontend/sema/consteval.cpp` were
  rechecked for the HIR layout handoff boundary:
  `struct_defs` plus `struct_def_owner_index`.

Authority classifications found:
- HIR type identity: `QualType` and `HirStructField::elem_type` still embed
  `TypeSpec`; HIR mostly preserves sema/parser type payload rather than owning
  one canonical type table. `Lowerer::fn_ptr_sig_from_decl_node` and struct
  field lowering use `ResolvedTypeTable`/`CanonicalType` when available for
  callable function-pointer signatures, but most ordinary HIR type refs remain
  `TypeSpec`-based.
- Template type identity: `HirTemplateParameterBindingKey` is the owner-aware
  identity for template type/NTTP bindings. `SpecializationKey` equality and
  hashing use structured owner plus ordered `SpecializationArgumentIdentity`
  values; `SpecializationKey::canonical` is retained as display/compatibility.
- Pending-work authority: `PendingTemplateTypeKey` is the dedup/progress key
  for deferred template type work. It combines kind, `specialization_type_*`
  identity for the pending `TypeSpec`, owner identity, type/NTTP binding
  identities, context, and span. `PendingTemplateTypeWorkItem::display_key`,
  `format_pending_type_ref_for_display`, and `encode_pending_type_ref` are
  diagnostic/display only.
- Template seed/instance authority: `InstantiationRegistry` uses
  `FunctionTemplateInstanceKey{primary_def, SpecializationKey}` when a
  primary owner exists. Legacy mangled-name dedup is explicitly limited to
  ownerless fallback. Template struct instances use
  `TemplateStructInstanceKey{primary_def, SpecializationKey}`.
- Record/layout identity: `HirRecordOwnerKey` is the structured HIR record
  owner key. `Module::struct_def_owner_index` maps owner keys to rendered
  tags, while `Module::struct_defs` and `struct_def_order` remain rendered-tag
  bridges for codegen, dumps, and no-owner compatibility. `compute_struct_layout`
  computes concrete layout on `HirStructDef`; base lookup prefers
  owner/text-id metadata but still stores `base_tags` as rendered strings.
- Member/method/static-member identity: `HirStructMethodLookupKey` and
  `HirStructMemberLookupKey` combine record owner key with member/method
  `TextId`; owner-key maps such as `struct_methods_by_owner_`,
  `struct_static_member_decls_by_owner_`,
  `struct_static_member_const_values_by_owner_`, and
  `struct_member_symbol_ids_by_owner_` are the structured authority when
  metadata is complete. Rendered tag/member maps are retained for no-owner
  compatibility and parity observation.
- Display spelling and route-local rendering: HIR still renders mangled
  template names, `format_type_for_specialization_display_key`,
  `nominal_type_suffix_for_mangling`, `type_suffix_for_mangling`,
  `build_template_mangled_name`, member-symbol strings like `tag::member`,
  and debug labels. These are output, ABI/mangling, diagnostic, or route-local
  bookkeeping unless used as explicit compatibility fallback.
- Compatibility bridges: structured misses commonly fail closed when complete
  owner metadata exists, then rendered fallback is only allowed for no-owner or
  explicitly self-consistent compatibility paths. Examples include declaration
  lookup in `Module`, record layout lookup in consteval, `resolve_typedef_to_struct`,
  static member lookup, member-symbol lookup, base layout lookup, and rendered
  template-origin recovery.

Highest-risk HIR gaps:
- HIR type refs remain `TypeSpec`-first. Aside from callable fn-ptr signatures,
  HIR does not consistently carry sema `CanonicalType` through ordinary
  `QualType`, field, local, global, parameter, and initializer paths. Any
  future fix must avoid treating raw `TypeSpec` spelling as equivalent to
  canonical identity.
- Template record owner keys still embed
  `HirRecordOwnerTemplateIdentity::specialization_key` as the serialized
  `SpecializationKey::canonical` compatibility string. This makes template
  record/layout ownership partially rendered even though `SpecializationKey`
  itself has structured owner/argument identity.
- Layout base identity is mixed. `HirStructDef::base_tags` is rendered, and
  `base_tag_text_ids` plus owner lookup recover structure when possible.
  `find_base_struct_def_for_layout` and related inheritance/static-member
  routes can still traverse rendered base tags after structured metadata is
  incomplete.
- Pending template work has a structured key, but the non-structured overloads
  of `record_pending_template_type` and `seed_pending_template_type` can still
  produce legacy binding identities when structured mirrors are incomplete.
  Structured seeding is used only when static context and binding counts prove
  parity.
- Template primary recovery still has rendered-origin bridges:
  `canonical_template_struct_primary`, `realize_template_struct`, and value-arg
  helpers can search by `tpl_struct_origin`, family-name suffixes, or rendered
  tag spelling when structured origin metadata is absent.
- Member/static-member lookup is mostly fail-closed for complete owner keys,
  but base traversal and template trait paths may re-enter rendered tag/member
  overloads. That is compatibility today, but it is a high-risk ambiguity for
  inherited members and instantiated static constexpr values.
- Ref-overload detection compares `TypeSpec` record identity by owner key when
  possible, then by `record_def`/`tag_text_id`, and finally returns true for
  no-structured-metadata record carriers. That no-metadata compatibility path
  can merge unrelated record parameter types if both sides lack owner data.
- Function pointer identity in HIR is split: callable signatures can carry
  `FnPtrSig::canonical_sig`, but many HIR maps and parity helpers still compare
  `TypeSpec` fields and `is_fn_ptr` flags. This mirrors the Step 1 fn-ptr risk.

## Suggested Next

Proceed to Step 3 by auditing LIR/BIR/backend type and layout authority,
especially where HIR `TypeSpec`, rendered struct tags, `HirRecordOwnerKey`,
layout metadata, and ABI/mangled spellings are converted into backend-facing
type/layout decisions.

## Watchouts

- Do not collapse all rendered HIR strings into one bug class. Some are
  display/mangling outputs, some are no-owner compatibility bridges, and a
  smaller set still participates in dedup/layout/lookup authority.
- `struct_def_owner_index` is the key record/layout bridge to track into
  later stages. If backend code consumes only rendered `struct_defs` tags,
  classify whether that is output spelling or semantic layout identity.
- Template record ownership is the clearest HIR ambiguity: structured
  specialization identity exists, but HIR record owner keys currently store
  the canonical display string for template instance identity.
- Owner-key fail-closed behavior is intentional. Avoid proposing fixes that
  re-open rendered fallback after complete owner metadata misses.
- The next audit should watch for places where `MemberSymbolId`, `LinkNameId`,
  or ABI names become backend authority instead of output identifiers.

## Proof

- No tests executed; packet proof was the required read-only audit evidence in
  `todo.md`.
- AST-backed inventory used:
  `c4c-clang-tool function-signatures` on
  `src/frontend/hir/impl/lowerer.hpp`; it returned partial local signatures
  and then reported a missing `source_profile.hpp` include path, so the audit
  proceeded with targeted direct reads and `rg` evidence instead of relying on
  the incomplete AST output.
