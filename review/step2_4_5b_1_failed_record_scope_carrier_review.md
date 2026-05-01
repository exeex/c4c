# Step 2.4.4.5B.1 Failed Record-Scope Carrier Review

Active source idea path: `ideas/open/139_parser_sema_rendered_string_lookup_removal.md`

Report path: `review/step2_4_5b_1_failed_record_scope_carrier_review.md`

Chosen base commit: `94e053b6d` (`[plan] Review failed record-scope carrier route`)

Reason: this commit is the lifecycle checkpoint that converted the failed record-scope implementation attempt into the current review-only Step 2.4.4.5B.1. It links the active idea, preserves the accepted Step 2.4.4.5B `base.cpp` deletions, and records that no implementation progress is claimed from the reverted attempt. Reviewing from this checkpoint avoids treating the reverted attempt as live diff.

Commit count since base: 0

Working tree: clean before review.

## Findings

### High: the next carrier is record-scope using-alias RHS metadata, not Step 2.4.4.5C

The two timeout fixtures still need `apply_alias_template_member_typedef_compat_type` because record-scope `using name = typename Owner<Args>::member;` stores only a `TypeSpec` in `member_typedef_types`. The structured alias-template member carrier can exist while parsing the RHS, but the record member typedef registration surface has nowhere to persist that carrier beside the alias name. Without the projector, the RHS no longer gets fabricated into `tpl_struct_origin + deferred_member_type_name`, so later record member typedef lookup has neither structured RHS metadata nor the old deferred-member `TypeSpec` bridge.

Evidence:

- `src/frontend/parser/impl/types/struct.cpp:740` parses record-scope `using` members.
- `src/frontend/parser/impl/types/struct.cpp:756` calls `parser.parse_type_name()` for the alias RHS.
- `src/frontend/parser/impl/types/struct.cpp:759` registers only the alias `TextId` and `TypeSpec`.
- `src/frontend/parser/impl/types/struct.cpp:760` and `src/frontend/parser/impl/types/struct.cpp:761` store only parallel alias name/type vectors.
- `src/frontend/parser/impl/declarations.cpp:257` defines the retained projector.
- `src/frontend/parser/impl/declarations.cpp:265` through `src/frontend/parser/impl/declarations.cpp:296` show why the projector works: it rewraps structured owner/member data into the old deferred-member `TypeSpec` shape that the record storage can carry.

Step 2.4.4.5C is adjacent but not the next exact route for these two fixtures. `register_record_member_typedef_bindings()` already publishes dependent/template record member typedef availability from the existing `Node` member typedef arrays. That step owns replacing rendered `source_tag::member` availability publication, especially for the three separate fixtures named in Step 2.4.4.5C. It does not solve the current earlier loss: the RHS of a record-scope using-alias has already been flattened to a `TypeSpec` before availability registration runs.

### Medium: the failed record-scope attempt likely targeted the right boundary but the wrong payload/persistence shape

The route should not be retried as a vague "record-scope carrier". The carrier must be a sidecar for record member typedef aliases that preserves the RHS semantic payload directly:

- alias member name: `TextId`
- owning record identity for the declared member alias: direct record node or `QualifiedNameKey`
- RHS owner: `QualifiedNameKey`
- RHS owner args: structured/substitutable parsed args
- RHS member: `TextId`

The sidecar needs to travel with `RecordBodyState` into `Node` record member typedef metadata, or an equivalent parser/Sema map keyed by record owner plus alias member `TextId`. The consumer then resolves `Record<Args>::alias` by reading that sidecar and applying the same structured owner/member resolution used by the alias-template carrier route, not by reconstituting a deferred `TypeSpec`.

Evidence:

- `src/frontend/parser/impl/declarations.cpp:166` through `src/frontend/parser/impl/declarations.cpp:206` already parse the right RHS carrier for top-level aliases before `parse_type_name()`.
- `src/frontend/parser/impl/declarations.cpp:1498` through `src/frontend/parser/impl/declarations.cpp:1552` show the top-level alias path has an active-context carrier handoff that record-scope `try_parse_record_using_member` lacks.
- `src/frontend/parser/impl/types/base.cpp:2089` through `src/frontend/parser/impl/types/base.cpp:2196` already contains the structured resolution logic for `ParserAliasTemplateInfo::member_typedef`: owner key, structured args, concrete instantiation/member `TextId`, and selected primary/specialization member typedefs.
- `src/frontend/parser/impl/types/base.cpp:3212` through `src/frontend/parser/impl/types/base.cpp:3221` registers concrete template-instantiation member typedefs by structured instantiation key plus member `TextId`; the next route should reuse that structured lookup class, not rendered splitting.

### Medium: `parse_dependent_typename_specifier` still explains the timeout path, but it should not be the new carrier source

The sampled path reaches `parse_dependent_typename_specifier` because record-scope using-alias parsing asks `parse_type_name()` to produce a complete `TypeSpec`. That function still contains rendered/deferred helper behavior for dependent template owner/member spellings. The next packet should not expand those helpers or seed metadata from their rendered token joins. It should parse/capture the RHS carrier before or alongside `parse_type_name()` at the record using-alias boundary, mirroring the top-level `try_parse_alias_template_member_typedef_info()` timing.

Evidence:

- `src/frontend/parser/impl/types/declarator.cpp:548` starts `parse_dependent_typename_specifier`.
- `src/frontend/parser/impl/types/declarator.cpp:565` through `src/frontend/parser/impl/types/declarator.cpp:578` reconstruct token spelling into `spelled_name`.
- `src/frontend/parser/impl/types/declarator.cpp:601` through `src/frontend/parser/impl/types/declarator.cpp:650` can manufacture a deferred owner/member `TypeSpec`.
- `src/frontend/parser/impl/types/declarator.cpp:733` through `src/frontend/parser/impl/types/declarator.cpp:782` retains another rendered/deferred preservation branch.
- `src/frontend/parser/impl/types/declarator.cpp:1940` through `src/frontend/parser/impl/types/declarator.cpp:1944` show why record `using` reaches this code: `parse_type_name()` is `parse_base_type()` plus declarator parsing.

## Answer To Review Question

The two timeout fixtures still need `apply_alias_template_member_typedef_compat_type` because the accepted Step 2.4.4.5B `base.cpp` deletions removed alias-template fallback paths, but record-scope using-alias registration still has no structured place to store `typename Owner<Args>::member` RHS metadata. The retained projector is bridging that gap by converting the already-structured RHS carrier into the old deferred-member `TypeSpec` fields that `member_typedef_types` can carry. The failed/reverted record-scope attempt did not change the durable storage/consumer contract enough to replace that projection, so deleting the projector leaves the record-scope path with a flattened or unresolved `TypeSpec` and no RHS carrier.

The next exact route is Step 2.4.4.5B.2 as a narrowed record-scope using-alias RHS sidecar carrier:

- Add a parser/Sema-owned sidecar for record-scope `using name = typename Owner<Args>::member;`.
- Capture it at `try_parse_record_using_member` before/alongside `parse_type_name()`, using the same structured parse timing as `try_parse_alias_template_member_typedef_info()`.
- Persist it with record member typedef metadata by record owner plus alias member `TextId`.
- Teach the record member typedef reader for `Record<Args>::name` to resolve through RHS owner `QualifiedNameKey`, structured/substitutable owner args, and RHS member `TextId`.
- Reuse existing structured alias-template/template-instantiation member typedef resolution surfaces where possible.
- Then retry deleting `apply_alias_template_member_typedef_compat_type`.

Do not jump to Step 2.4.4.5C yet. 5C remains needed for dependent/template record member typedef availability, but it is not the precise carrier missing for the two current timeout fixtures.

## Rejected Routes

- Rendered or deferred `TypeSpec` rewrapping.
- Parsing `debug_text`.
- Splitting rendered `Owner::member` strings.
- `qualified_name_from_text`, `qualified_alias_name`, `tpl_struct_origin`, or `deferred_member_type_name` as carrier sources.
- Named-test shortcuts or fixture-specific branches.
- Expectation downgrades or unsupported reclassification.
- Reintroducing the deleted `base.cpp` alias-template context fallback or dependent rendered/deferred `TypeSpec` projection.

## Judgments

Idea-alignment judgment: matches source idea.

Runbook-transcription judgment: plan is lossy but usable; `todo.md` should be updated with the narrowed B.2 sidecar route before implementation resumes.

Route-alignment judgment: narrow next packet.

Technical-debt judgment: action needed.

Validation sufficiency: needs broader proof after implementation; use fresh build plus `ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_'` and explicitly confirm the two timeout fixtures pass with `apply_alias_template_member_typedef_compat_type` deleted.

Reviewer recommendation: narrow next packet. Implement Step 2.4.4.5B.2 as record-scope using-alias RHS sidecar metadata, then retry projector deletion. Do not proceed to Step 2.4.4.5C until this carrier has either covered the two fixtures or exposed a different exact structured boundary.
