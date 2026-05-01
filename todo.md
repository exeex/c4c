# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.4.1
Current Step Title: Inventory Live Member-Typedef Mirror Consumers

## Just Finished

Step 2.4.1 inventory completed for the live member-typedef mirror writers and
readers protected by rendered `owner::member` storage. No implementation files,
tests, or proof logs were changed.

Live writer inventory:

- `src/frontend/parser/impl/types/struct.cpp` record-body parsing stores
  member typedef declarations into `Node::member_typedef_names` and
  `Node::member_typedef_types` from `try_parse_record_using_member` and
  `try_parse_record_typedef_member`; strongest carrier is member typedef arrays
  on the direct record `Node`, with member `TextId` recoverable at registration
  time.
- `register_record_member_typedef_bindings` writes non-template record members
  into `binding_state_.struct_typedefs` under
  `record_member_typedef_key_in_context(context_id, record_text_id,
  member_text_id)`; strongest carrier is namespace context id plus
  `QualifiedNameKey`.
- The same registration writes template/dependent record members into
  `template_state_.dependent_record_member_typedefs_by_key`; strongest carrier
  is `QualifiedNameKey`, but it still depends on record/member text ids rather
  than a first-class owner-member metadata object.
- Template instantiation cloning in
  `src/frontend/parser/impl/types/base.cpp` writes substituted member typedefs
  into both the instantiated record's member typedef arrays and
  `template_instantiation_member_typedefs_by_key`; strongest carrier is
  template-instantiation metadata
  (`TemplateInstantiationKey` plus member `TextId`).
- Alias-template RHS capture in
  `src/frontend/parser/impl/declarations.cpp` writes
  `ParserAliasTemplateMemberTypedefInfo` into `ParserAliasTemplateInfo`;
  strongest carrier is structured alias-template metadata: owner
  `QualifiedNameKey`, parsed owner args, and member `TextId`.

Live reader inventory:

- `Parser::find_typedef_type(TextId)` still treats scoped text containing
  `::` as semantic input by building `known_fn_name_key(0, name_text_id)` and
  then querying the structured/dependent tables. Carrier classification:
  missing metadata at this API boundary; do not convert by parsing rendered
  `owner::member` text back into owner/member identity.
- `Parser::find_typedef_type(const QualifiedNameKey&)` reads
  `struct_typedefs` first and then
  `dependent_record_member_typedefs_by_key`; strongest carrier is
  `QualifiedNameKey`.
- `Parser::find_template_instantiation_member_typedef_type` reads
  `template_instantiation_member_typedefs_by_key`; strongest carrier is
  template-instantiation metadata plus member `TextId`.
- `probe_qualified_type` in `types_helpers.hpp` first resolves the owner through
  structured record authority and scans the owner `Node` member typedef arrays;
  strongest carrier is direct record metadata / `TypeSpec::record_def` path
  once the owner is resolved.
- `parse_declarator` qualified typedef lookup in `types/declarator.cpp` first
  uses `qualified_name_key(qn)` and then reconstructs a
  `record_member_typedef_key_in_context` from owner namespace context plus
  owner/member `TextId`; strongest carrier is `QualifiedNameKey` plus namespace
  context id.
- The nested-owner declarator fallback scans direct record `Node`
  `member_typedef_*` arrays and populates `struct_typedefs` for the structured
  key after a hit; strongest carrier is direct record/member metadata, with the
  structured table as a cache.
- `parse_type_name` member-suffix handling in `types/base.cpp` reads direct
  record member arrays, selected specialization member arrays,
  `template_instantiation_member_typedefs_by_key`, and recursive base records;
  strongest carriers are `TypeSpec::record_def`, member typedef arrays, and
  template-instantiation metadata, but its local `lookup_typedef_for_name`
  fallback can still call `find_typedef_type(TextId)` for scoped rendered text.
- Alias-template member-typedef resolution in `types/base.cpp` reads
  `ParserAliasTemplateMemberTypedefInfo`, template-instantiation member tables,
  and selected specialization member arrays; strongest carrier is
  alias-template/template-instantiation metadata.
- Expression disambiguation in `expressions.cpp` uses
  `probe_qualified_type`, direct owner record member arrays, then a tentative
  `parse_base_type` probe and compares `last_resolved_typedef_text()` against
  rendered qualified spellings. Carrier classification: direct record metadata
  is available for the first branch; the tentative parse/text comparison branch
  is a missing-metadata blocker and must not be expanded as a structured route.
- Existing focused parser tests cover stale rendered owner/member disagreement,
  `record_member_typedef_key_in_context`, direct member arrays, template
  instantiation member typedefs, and alias-template member typedef carriers.
  They are evidence for classification only; this packet does not claim code
  proof or implementation progress.

## Suggested Next

Start Step 2.4.2 with the smallest direct-carrier conversion: remove one
consumer's dependency on rendered scoped typedef lookup by making the direct
record/member-array path the authority before table lookup.

Recommended first target: the `probe_qualified_type` /
expression-disambiguation direct-owner branch, because it already has the owner
`Node*`, member `TextId`, and member typedef arrays. Convert that route to
return/use the direct member typedef payload without falling through to the
tentative rendered-text probe when the direct owner/member carrier is present.
Keep the scoped-text fallback recorded as a metadata blocker instead of parsing
`owner::member` text into identity.

## Watchouts

- Do not invent another Step 2.3 packet without a named concrete parser/Sema
  rendered semantic authority route.
- Step 2.4.1 was inventory-only. Do not delete the `owner::member` mirror,
  delete APIs, or claim storage narrowing from this packet.
- Explicitly reject helpers that accept rendered `owner::member` text,
  `std::string`, or `std::string_view` qualified text and parse it back into
  owner/member identity.
- `find_typedef_type(TextId)` scoped-string handling and the
  `expressions.cpp` tentative parse/text-comparison branch are metadata
  blockers, not acceptable conversion templates.
- The known qualified member typedef blocker from Step 2.3 remains relevant:
  instantiated member typedefs such as `ns::holder_T_int::type` need concrete
  owner-member `TextId` or equivalent domain metadata. Full `TypeSpec::tag`
  field deletion belongs to idea 141, not this packet.
- HIR-only rendered lookup cleanup remains out of scope for idea 139 and
  belongs to idea 140.

## Proof

No build proof required or run; this was inventory-only and changed only
`todo.md`.

Validation run: `git diff --check`.
