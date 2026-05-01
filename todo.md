# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.4.4.3
Current Step Title: Convert C-Style Cast Type-Id Member-Typedef Consumer

## Just Finished

Step 2.4.4.1 re-audited the live member-typedef mirror routes after the
Step 2.4.2 and Step 2.4.3 conversions. Current writers/readers are:

- Record-body writers: `try_parse_record_using_member` and
  `try_parse_record_typedef_member` collect member aliases into
  `RecordBodyState::member_typedef_names/member_typedef_types`; the durable
  carrier is the direct record member arrays copied by
  `store_record_body_members` onto `Node::member_typedef_names` and
  `Node::member_typedef_types`.
- Record binding writers: `register_record_member_typedef_bindings` publishes
  non-template record aliases through `QualifiedNameKey` from
  `record_member_typedef_key_in_context(context_id, record_text_id,
  member_text_id)` and publishes dependent/template-context aliases through
  the same structured key into `dependent_record_member_typedefs_by_key`.
- Parser lookup readers: `find_typedef_type(QualifiedNameKey)` first reads the
  structured typedef table, then the dependent record-member table. The
  declaration/typename routes in `types/declarator.cpp` use
  `qualified_name_key(qn)` or `record_member_typedef_key_in_context` rather
  than reconstructing from rendered `owner::member` text.
- Parser direct record readers: `qualified_type_record_definition_from_structured_authority`
  and `probe_qualified_type` still read direct `Node::member_typedef_*`
  arrays from a resolved owner `record_def`/record definition. This is a real
  structured carrier and is not the rejected rendered-string route.
- Parser namespace-context readers: the Step 2.4.3 helper now resolves qualified
  owner typedefs through `TypeSpec` qualifier `TextId`s/global qualification
  and `QualifiedNameKey`, or through `namespace_context_id` plus
  `struct_typedef_key_in_context`; only plain unqualified owner names still use
  visible typedef lookup.
- Template-instantiation writer: direct template instantiation clones member
  typedef arrays onto the instantiated `Node` and writes
  `template_instantiation_member_typedefs_by_key` through
  `register_template_instantiation_member_typedef_binding`. Its carrier is
  structured: `ParserTemplateState::TemplateInstantiationKey` for the concrete
  owner plus member `TextId`.
- Template-instantiation parser readers: `parse_base_type` has readers that
  build the same structured `TemplateInstantiationKey` from a primary template
  key and parsed/direct actual-argument keys, then read by member `TextId`.
  This confirms the parser call sites have a structured owner/member carrier
  before API deletion.
- Alias-template member typedef metadata: `ParserAliasTemplateMemberTypedefInfo`
  carries `owner_key`, parsed owner args, and `member_text_id`; alias-template
  expansion can either defer via `TypeSpec::tpl_struct_origin`,
  `tpl_struct_args`, and `deferred_member_type_text_id`, or read the structured
  template-instantiation table directly.
- HIR readers: `resolve_struct_member_typedef_if_ready` can resolve through
  `TypeSpec::record_def` first, then `tpl_struct_origin` plus structured
  template-argument metadata, and finally the legacy `tag/member` string API.
  `resolve_struct_member_typedef_type(tag, member, out)` still reads
  `struct_def_nodes_` by realized tag and member spelling, then direct
  `Node::member_typedef_*` arrays, selected template-pattern metadata, and base
  tags. This is the remaining metadata blocker for deleting the mirror API.

## Suggested Next

Step 2.4.4.2 is retired because the Step 2.4.4.1 audit found the planned
parser template-instantiation carrier already exists:
`TemplateInstantiationKey` plus member `TextId`.

Next executor packet should start Step 2.4.4.3. Convert the C-style
cast/type-id parser consumer for non-template record-body member typedefs so it
resolves `Box::AliasL` / `Box::AliasR` style references through structured
record/member metadata instead of the rendered generic `owner::member` typedef
table. Do not make HIR resolver API migration the next packet for idea 139.

## Watchouts

- Do not delete the `owner::member` mirror, delete APIs, or claim storage
  narrowing from this packet.
- Continue rejecting local reconstruction from rendered `mangled` or
  `owner::member` strings. The live parser-side template-instantiation table
  already has a structured owner/member key; the HIR tag/member API is the
  blocker, not missing parser metadata.
- HIR `resolve_struct_member_typedef_type(std::string tag, std::string member,
  ...)` cleanup belongs to
  `ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md`; do not widen
  idea 139 into `src/frontend/hir`.
- `find_typedef_type(TextId)` still contains scoped-string compatibility for
  pre-existing callers, but the Step 2.4.3 member-typedef route no longer
  depends on that for scoped owner names.
- HIR callers that currently pass `ts.tag` and
  `ts.deferred_member_type_name` should be migrated carefully: some have
  `record_def`, `tpl_struct_origin`, `tpl_struct_args`, and
  `deferred_member_type_text_id`; others only have realized tag/member strings
  and need metadata plumbing before the legacy API can be deleted.
- HIR-only rendered lookup cleanup remains out of scope for idea 139 and
  belongs to idea 140.

## Proof

Delegated proof for this inventory-only packet:

`git diff --check > test_after.log 2>&1`

Result: passed.
