# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Review Artifact Path: review/step2_4_5b_1_failed_record_scope_carrier_review.md
Current Step ID: Step 2.4.4.5B.2
Current Step Title: Implement Reviewed Record-Scope Using-Alias RHS Sidecar Carrier

## Just Finished

Step 2.4.4.5B.1 review is complete. The reviewer chose the next packet:
continue narrowly to Step 2.4.4.5B.2 as record-scope using-alias RHS sidecar
metadata, not Step 2.4.4.5C.

Review decision:

- The two timeout fixtures still need `apply_alias_template_member_typedef_compat_type`
  because record-scope `using name = typename Owner<Args>::member;` currently
  persists only alias `TextId` plus flattened `TypeSpec` in record member
  typedef metadata.
- The missing carrier is the RHS semantic payload for the record-scope alias,
  captured at the record `using` boundary before or alongside `parse_type_name()`.
- Step 2.4.4.5C remains adjacent future work for dependent/template record
  member typedef availability, but it is not the precise carrier missing for
  the two current timeout fixtures.
- The failed/reverted Step 2.4.4.5B.1 implementation attempt is not accepted
  implementation progress.

Accepted partial Step 2.4.4.5B results remain in force: the alias-template
context fallback and dependent rendered/deferred `TypeSpec` projection in
`base.cpp` stay deleted.

## Suggested Next

Next packet is implementation Step 2.4.4.5B.2: add a parser/Sema-owned sidecar
for record-scope `using name = typename Owner<Args>::member;`.

Exact sidecar route:

- Capture the RHS carrier at `try_parse_record_using_member` before or alongside
  `parse_type_name()`, using the same structured parse timing as the top-level
  alias-template member typedef carrier.
- Persist sidecar metadata with record member typedef metadata by record owner
  plus alias member `TextId`.
- Sidecar payload must preserve:
  - alias member name as `TextId`
  - owning record identity for the declared member alias as direct record owner
    metadata or `QualifiedNameKey`
  - RHS owner as `QualifiedNameKey`
  - RHS owner args as structured/substitutable parsed args
  - RHS member as `TextId`
- Teach the record member typedef reader for `Record<Args>::name` to read this
  sidecar and resolve through RHS owner `QualifiedNameKey`, structured or
  substitutable owner args, and RHS member `TextId`.
- Reuse existing structured alias-template/template-instantiation member typedef
  resolution surfaces where possible.
- After this sidecar covers the two timeout fixtures, retry deletion of
  `apply_alias_template_member_typedef_compat_type` in Step 2.4.4.5B.3.

Owned implementation surfaces for the executor:

- Record-scope `using` parsing around `try_parse_record_using_member`.
- Record body/member typedef storage that currently carries alias `TextId` plus
  `TypeSpec`.
- Parser-to-Sema or `Node` record member typedef metadata needed to persist the
  sidecar by record owner plus alias member `TextId`.
- Record member typedef lookup/reader path for `Record<Args>::name`.
- Existing structured alias-template/template-instantiation member typedef
  resolver code only as needed for reuse, not broad redesign.

Stop and update `todo.md` instead of broadening the packet if the sidecar cannot
be attached at this boundary or if resolving the two fixtures requires a
different exact structured metadata owner.

## Watchouts

- Do not jump to Step 2.4.4.5C in this packet.
- Do not seed the sidecar from rendered/deferred `TypeSpec` fields,
  `tpl_struct_origin`, `deferred_member_type_name`, `qualified_name_from_text`,
  `qualified_alias_name`, `debug_text`, or split rendered `Owner::member`
  spelling.
- Do not parse `debug_text`, split rendered owner/member strings, add
  fixture-specific branches, downgrade expectations, mark fixtures unsupported,
  or reintroduce the deleted `base.cpp` fallback/projection routes.
- Do not count helper renames or another wrapper around
  `apply_alias_template_member_typedef_compat_type` as progress.
- Keep rendered owner/member spelling only for diagnostics, display, debug
  output, mangling, or final emitted text.

## Proof

Executor proof command:

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_' >> test_after.log 2>&1`

Required proof result:

- Build succeeds.
- `cpp_positive_sema_` passes.
- The two timeout fixtures
  `cpp_positive_sema_iterator_concepts_following_hash_base_parse_cpp` and
  `cpp_positive_sema_stl_iterator_then_max_size_type_parse_cpp` pass with
  `apply_alias_template_member_typedef_compat_type` deleted or ready for the
  immediate Step 2.4.4.5B.3 deletion retry.
- Fresh canonical proof is written to `test_after.log`.
