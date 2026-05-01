# Parser/Sema Rendered String Lookup Removal Runbook

Status: Active
Source Idea: ideas/open/139_parser_sema_rendered_string_lookup_removal.md

## Purpose

Finish the parser/Sema cleanup by deleting rendered-string semantic lookup
routes instead of renaming, reclassifying, or preserving them as compatibility
overloads.

Goal: parser/Sema semantic lookup must use direct AST links, declaration/owner
objects, namespace context ids, `TextId`, `QualifiedNameKey`,
`TypeSpec::record_def`, or Sema structured owner keys where those carriers are
available. Semantic lookup APIs must not take `std::string`,
`std::string_view`, rendered spelling, or `TextId` plus fallback spelling.

## Core Rule

Parser/Sema semantic lookup interfaces must not accept rendered strings,
`std::string`, `std::string_view`, or fallback spelling arguments. Collapse any
string-plus-structured overload family to the `TextId`, `QualifiedNameKey`,
owner, declaration, namespace, or other domain-key API. Names containing
`fallback` or `legacy` are reject signals when they preserve semantic lookup
through spelling.

If structured metadata is missing, repair the producer or open a metadata idea.
Do not keep a string rediscovery route as the plan outcome.

## Read First

- `ideas/open/139_parser_sema_rendered_string_lookup_removal.md`
- `src/frontend/parser`
- `src/frontend/sema`
- Parser typedef, value, tag, template, NTTP-default, and known-function lookup
  paths
- Sema owner/member/static/consteval lookup paths
- Parser-to-Sema AST and semantic handoff surfaces
- Parser/Sema APIs that take `std::string`, `std::string_view`, rendered
  spelling, or `TextId` plus a fallback spelling
- Parser/Sema semantic helpers with `fallback` or `legacy` in their names

## Current Targets

- Parser string-keyed semantic lookup maps and rendered-name probes
- Parser semantic lookup APIs with string/string_view parameters,
  string-compatible overloads, or fallback spelling parameters
- Parser-owned `fallback`/`legacy`/`compatibility` routes where structured
  metadata already exists, including visible-name compatibility spelling,
  visible typedef fallback depth/guards, known-function compatibility probes,
  and legacy var/type binding caches.
- Parser call sites of text helpers that pass rendered fallback spelling for
  semantic lookup. The helper primitives are not automatically violations; the
  semantic call sites must be collapsed when a `TextId`, `QualifiedNameKey`,
  namespace context id, direct record/declaration, or owner key is available.
- Parser call sites that render `QualifiedNameKey` and then re-enter
  string lookup
- Sema consteval, owner, member, and static lookup routes that consult rendered
  names after structured keys exist
- Sema semantic lookup overload families that preserve both string and
  structured routes
- Sema-owned `fallback`/`legacy`/`compatibility` routes where producer
  metadata is in parser/Sema scope, including structured owner/member/static
  lookup, rendered global/enum compatibility indexes, and same-stage consteval
  value/type fallback after metadata producers are complete.
- Parser-to-Sema metadata handoff for `TextId`, namespace context, record
  identity, `TypeSpec::record_def`, declaration objects, and owner keys
- Focused frontend tests for structured-vs-rendered disagreement cases

## Non-Goals

- Do not change HIR, LIR, BIR, or backend implementation except to record a
  separate metadata handoff blocker under `ideas/open/`.
- Do not remove diagnostic, source-spelling, dump, final emitted text, or
  ABI/link-visible strings.
- Do not weaken supported behavior, mark tests unsupported, or use
  testcase-shaped shortcuts.
- Do not treat helper renaming, comment changes, or wrapper extraction as
  lookup removal.
- Do not treat a `fallback` or `legacy` semantic route as acceptable quarantine.
- Do not infer namespace, owner, declaration, record, template, or value
  semantics from `TextId`; use a richer domain key when those semantics matter.

## Working Model

Parser and Sema lookup should follow this order:

1. Use direct AST links, declaration/owner objects, Sema structured owner keys,
   `QualifiedNameKey`, namespace-aware keys, or other domain semantic carriers.
2. Use `TextId` only where text identity is the correct lookup domain. A
   `map<TextId>` or `set<TextId>` is correct when the intended key is text
   identity itself.
3. Collapse semantic lookup overload families to the structured/domain-key
   route; do not keep a parallel string or string_view route.
4. Use rendered strings only for non-semantic output: source spelling,
   diagnostics, display, dumps, final emitted text, and ABI/link spelling.
5. When a semantic lookup cannot be converted because metadata is missing,
   create a new open idea for that producer/consumer contract and remove the
   string rediscovery goal from this packet.

## Execution Rules

- Start each packet by naming the exact rendered-string semantic route being
  removed.
- The packet must either delete that route, replace it with structured
  metadata, or open a metadata blocker idea.
- Remove semantic lookup APIs that accept `std::string`, `std::string_view`, or
  fallback spelling. For example,
  `has_typedef_name(TextId name_text_id, std::string_view fallback_name)` must
  become `has_typedef_name(TextId name_text_id)`.
- Delete parser/Sema semantic helpers named with `fallback` or `legacy` when
  they preserve string lookup compatibility.
- Do not present renamed helpers or newly labeled string wrappers as progress.
- Tests should cover same-feature drifted-string behavior, not only one narrow
  testcase.
- Update `todo.md` for packet progress; rewrite this runbook only when the
  route contract changes.
- Step 2 is now split into numbered parser substeps. Execute one substep per
  packet unless the supervisor explicitly delegates a broader review-only or
  lifecycle-only action.

## Known Blockers

- Parser const-int cleanup is parked at the HIR metadata boundary found in
  commit `28c1e5c5`: deleting the rendered-name
  `eval_const_int(..., const std::unordered_map<std::string, long long>*)`
  compatibility overload requires HIR `NttpBindings` metadata migration.
  Keep parser-owned callers on the `TextId`/structured map route, but do not
  pull `src/frontend/hir` carrier migration into this plan. Route that work
  through `ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md` or a
  narrower HIR metadata idea if the supervisor switches scope.
- HIR-only rendered lookup cleanup is not part of this active plan. Routes such
  as `find_function_by_name_legacy`, `find_global_by_name_legacy`,
  `has_legacy_mangled_entry`, `ModuleDeclLookupAuthority::LegacyRendered`,
  `declaration_fallback`, rendered compatibility indexes, HIR
  `NttpBindings`, and HIR `fallback_*` owner/member/tag recovery belong in
  `ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md`.
- Removing `TypeSpec::tag` as a field is not part of this active plan. Idea
  139 may move parser/Sema-owned call sites to `tag_text_id`, `record_def`, or
  explicit owner/member/template metadata, but the field deletion itself and
  cross-stage migration belong in
  `ideas/open/141_typespec_tag_field_removal_metadata_migration.md`.

## Steps

### Step 1: Inventory Rendered-String Semantic Authority

Goal: identify parser and Sema paths where rendered strings still decide
semantic lookup.

Primary target: `src/frontend/parser` and `src/frontend/sema`.

Actions:

- Search for string-keyed semantic maps, rendered-name probes, string or
  `std::string_view` semantic lookup parameters, `TextId` plus fallback
  spelling APIs, and helper paths that re-enter lookup through spelling.
- Search parser/Sema semantic APIs for `fallback` and `legacy` names.
- Classify only two outcomes for each semantic route: removable now, or blocked
  by missing metadata.
- Trace parser typedef, value, tag, template, NTTP-default, and known-function
  lookup paths.
- Trace Sema owner/member/static/consteval lookup paths.
- Choose the smallest first packet that deletes or structurally replaces one
  real string authority route.

Completion check:

- `todo.md` records the selected first removal packet and proof command.
- The inventory does not bless semantic string lookup as a retained outcome.
- The inventory identifies overload families to collapse and fallback/legacy
  API names to remove.

### Step 2.1: Remove Parser Template And NTTP Default Rendered Mirrors

Goal: parser template and NTTP-default lookup must prefer `QualifiedNameKey`,
`TextId`, and structured token caches without consulting rendered mirror keys
as semantic authority.

Primary target: `src/frontend/parser/impl/types/template.cpp` and parser
template-state APIs.

Actions:

- Inspect `template_instantiation_name_key`, template primary/specialization
  registration, NTTP-default token caching, and deferred NTTP-default
  evaluation.
- Delete rendered mirror lookups when the caller already has a
  `QualifiedNameKey`, namespace context id, template `TextId`, or structured
  NTTP-default key.
- Collapse overloads that take both a structured template key and a rendered
  template name when the rendered argument is used for lookup.
- Keep rendered template spelling only for mangling, diagnostics, debug text,
  or final emitted names.
- Add focused parser tests where drifted rendered template spelling must not
  affect template lookup or NTTP-default evaluation.

Completion check:

- Covered template and NTTP-default paths no longer use rendered mirror keys as
  alternate semantic authority after a valid structured key exists.
- Covered parser template lookup APIs no longer accept rendered spelling for
  semantic lookup.
- Tests prove structured template metadata wins over drifted rendered spelling.
- Narrow parser tests and a fresh build pass, with fresh canonical
  `test_after.log`, are produced by the executor.

### Step 2.2: Complete Parser VisibleName Projection Overload Cleanup

Goal: delete parser lookup overloads whose only purpose is projecting
structured lookup results into rendered strings, without claiming that deletion
as removal of the remaining semantic spelling authority.

Primary target: parser `VisibleNameResult` lookup APIs and focused parser tests
that used `std::string*` output projection overloads.

Actions:

- Delete `std::string*` projection overloads for parser value, type, and
  concept lookup when callers can keep `VisibleNameResult` until a display
  spelling is explicitly needed.
- Keep `visible_name_spelling(...)` only as an explicit display/diagnostic/test
  projection, not as a semantic lookup carrier.
- Update tests to assert structured lookup results first and project spelling
  only for display assertions.
- Do not count this checkpoint as completion of parser declarator, value,
  known-function, or Sema spelling/fallback removal.
- Do not claim a fresh proof artifact unless canonical `test_after.log` exists
  for the delegated proof command.

Completion check:

- Parser `std::string*` output projection overloads for the covered
  `VisibleNameResult` lookup family are deleted.
- Tests no longer require semantic lookup APIs to return rendered spelling
  through output parameters.
- `todo.md` records that remaining semantic spelling/fallback authority is
  still open.

### Step 2.3: Remove Remaining Parser Semantic Spelling And Fallback Authority

Goal: parser declarator, value, known-function, and visible-type lookup must
not recover semantic identity by projecting structured lookup results to
rendered spelling and then re-entering lookup through `TextId` or fallback
strings.

Primary target: parser declaration and statement paths that register or query
values, function declarations, known functions, qualified declarator names, and
visible types.

Actions:

- Inspect `resolve_visible_type(...)`, qualified declarator parsing,
  known-function registration, static member/value lookup, and expression owner
  lookup call sites.
- Delete the production route where `lookup_using_value_alias(...)` returns a
  `VisibleNameResult`, `visible_name_spelling(...)` projects it to a rendered
  string, the caller recovers a `TextId`, and typedef/type lookup is driven by
  that recovered spelling.
- Remove or replace parser semantic lookup parameters that still accept
  `std::string_view` spelling or fallback strings, including
  `lookup_using_value_alias`, `lookup_value_in_context`,
  `lookup_type_in_context`, and `lookup_concept_in_context`.
- Remove or replace parser-owned `fallback`/`legacy`/`compatibility` named
  semantic routes when the caller already has a real carrier, including legacy
  var/type binding caches, visible typedef fallback guards, visible-name
  compatibility spelling, and known-function compatibility probes.
- Convert lookup authority to `TextId`, namespace context ids,
  `QualifiedNameKey`, direct AST links, declaration objects, or
  `VisibleNameResult` fields where those carriers are available.
- Treat `kInvalidText` plus rendered fallback recovery as a semantic lookup
  route unless the retained string use is visibly diagnostic/display-only.
- Keep rendered names only for source spelling, diagnostics, mangling, ABI/link
  names, or final emitted text.
- Add focused parser tests for same-feature drifted rendered spelling in the
  covered value/type/known-function routes.

Completion check:

- Covered declarator/value/known-function/visible-type paths no longer use
  rendered spelling as alternate semantic authority after structured metadata
  exists.
- Covered parser APIs no longer accept string/string_view or fallback spelling
  parameters for semantic lookup.
- Tests prove structured metadata wins over drifted rendered spelling.
- Narrow parser tests and a fresh build pass, with fresh canonical
  `test_after.log`, are produced by the executor.

### Step 2.4.1: Inventory Live Member-Typedef Mirror Consumers

Goal: identify every remaining consumer protected by the live
`owner::member` typedef mirror before deleting or shrinking it.

Primary target: parser member-typedef registration and lookup routes,
including `register_struct_member_typedef_binding(owner, member, type)`,
`TypeSpec::record_def`, member typedef arrays, namespace context ids,
`QualifiedNameKey`, direct record/declaration metadata, and any call site that
still reaches semantic lookup through rendered scoped text.

Actions:

- Inventory the live mirror writers and readers, including template
  instantiation, record body parsing, qualified type lookup, declarator lookup,
  expression owner/type probes, and focused frontend tests that mention member
  typedefs.
- Classify each live consumer by the strongest available structured carrier:
  `TypeSpec::record_def`, member typedef arrays on a `Node`, namespace context
  id, `QualifiedNameKey`, direct record/declaration metadata, or only rendered
  scoped spelling.
- Record consumers that lack a structured carrier as metadata blockers instead
  of routing them through rendered text.
- Preserve or add focused parser tests only for one classified carrier at a
  time; do not use one large runtime case as the sole proof.
- Explicitly reject the previously reviewed route from
  `review/step2_4_member_typedef_conversion_review.md`: do not introduce a
  helper that accepts rendered `owner::member` text, `std::string`, or
  `std::string_view` qualified text and parses it back into owner/member
  identity.

Completion check:

- `todo.md` records the live mirror consumers, their carrier classification,
  and the first smallest consumer to convert.
- The next code packet is scoped to one carrier class, not deletion of the
  whole mirror.
- No helper taking rendered qualified text is proposed as structured progress.
- Lifecycle-only inventory rewrite does not claim code proof.

### Step 2.4.2: Prove Direct Record Member-Typedef Lookup

Goal: make direct record/member metadata own one member-typedef consumer before
changing mirror storage.

Primary target: consumers that already have `TypeSpec::record_def`, a direct
record `Node*`, or declaration metadata for the owner.

Actions:

- Convert one reachable consumer to scan member typedef arrays from the direct
  record/declaration carrier.
- Compare member identity through `TextId` or direct member metadata when that
  is the actual domain; do not fall back to `std::string_view` member equality.
- Keep rendered owner or member spelling only for diagnostics, display, debug,
  mangling, or final emitted text.
- Add or keep a focused parser disagreement test proving direct record metadata
  wins over stale rendered `owner::member` mirror storage.

Completion check:

- The converted consumer starts from direct record/declaration metadata and
  does not parse rendered qualified text.
- The covered test fails on stale mirror authority and passes through the
  direct carrier.
- Narrow parser tests and a fresh build pass, with fresh canonical
  `test_after.log`, are produced by the executor.

### Step 2.4.3: Prove Qualified Key Or Namespace-Context Lookup

Goal: convert one member-typedef consumer that lacks direct record metadata but
already has namespace-aware structured identity.

Primary target: call sites with an existing `QualifiedNameKey`, namespace
context id plus `TextId`, or parser qualified-name carrier.

Actions:

- Start from the existing qualified/domain key supplied by the caller; do not
  render it and then parse the rendered text back into a key.
- Route lookup through structured typedef maps or record/tag metadata keyed by
  namespace context and member text identity.
- Delete or narrow any overload whose only purpose is accepting rendered
  qualified spelling for this semantic lookup.
- Add or keep one focused disagreement test where stale rendered scoped
  spelling disagrees with the structured qualified carrier.

Completion check:

- The covered consumer no longer consults rendered `owner::member` spelling as
  semantic authority after a structured qualified carrier exists.
- No parser semantic API for the route accepts `std::string`,
  `std::string_view`, rendered qualified text, or fallback spelling.
- Narrow parser tests and a fresh build pass, with fresh canonical
  `test_after.log`, are produced by the executor.

### Step 2.4.4.1: Re-Audit Member-Typedef Mirror Writers And Readers

Goal: re-establish the live `owner::member` typedef mirror inventory after the
Step 2.4.3 reader conversions, before deleting APIs or changing mirror storage.

Primary target: the record-body writer, template-instantiation writer, and any
remaining lookup path backed by rendered scoped typedef storage.

Actions:

- Re-run the Step 2.4.1 inventory and classify each remaining writer and
  reader by its real structured carrier: direct record/member metadata,
  `QualifiedNameKey`, namespace context plus `TextId`, template-instantiation
  metadata, or missing metadata.
- Confirm whether `src/frontend/parser/impl/types/base.cpp` has a structured
  owner/member carrier available at the template-instantiation call site before
  any API deletion is attempted.
- Treat local reconstruction from a rendered `mangled` or `owner::member`
  string as the same rejected route as a wrapper accepting rendered qualified
  text.
- Record missing carrier cases in `todo.md` as blockers or separate metadata
  idea candidates; do not delete the mirror while semantic consumers still need
  rendered scoped storage.

Completion check:

- `todo.md` lists each live writer/reader and names the next single structural
  conversion or metadata blocker.
- No wrapper deletion, helper rename, or local rendered-string split is counted
  as progress.
- Lifecycle-only inventory rewrite does not claim code proof.

### Step 2.4.4.2: Retired - Template-Instantiation Carrier Already Exists

Goal: record the Step 2.4.4.1 route correction and avoid re-adding parser
metadata that the audit found already present.

Primary target: no implementation packet. This step is retired by the
Step 2.4.4.1 inventory.

Audit result:

- Direct template instantiation already writes member typedefs through
  `template_instantiation_member_typedefs_by_key`.
- The writer and parser readers use
  `ParserTemplateState::TemplateInstantiationKey` plus member `TextId`, not
  rendered `owner::member` spelling, as the concrete owner/member carrier.
- Alias-template member typedef metadata also preserves owner key, parsed owner
  arguments, and member `TextId` for parser/Sema routing.
- The remaining blocker identified by the audit is the HIR
  `resolve_struct_member_typedef_type(std::string tag, std::string member, ...)`
  API and callers that only have realized tag/member strings. That cleanup is
  outside idea 139 and belongs to
  `ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md`.

Actions:

- Do not add another parser-owned
  `TemplateInstantiationKey concrete_owner + TextId member_text_id` carrier.
- Do not convert the next packet into HIR API migration.
- Continue idea 139 with the next parser/Sema member-typedef consumer that
  still depends on rendered scoped typedef storage.

Completion check:

- `todo.md` preserves the Step 2.4.4.1 inventory and removes HIR cleanup as
  the suggested next packet.
- The active execution pointer skips to Step 2.4.4.3.
- HIR member-typedef resolver cleanup remains routed to idea 140.

### Step 2.4.4.3: Convert C-Style Cast Type-Id Member-Typedef Consumer

Goal: make C-style cast/type-id parsing resolve non-template record-body member
typedefs through structured record/member metadata before the rendered
`owner::member` mirror is deleted.

Primary target: the parser type-id and C-style cast path that currently depends
on record-body rendered scoped typedef entries such as `Box::AliasL` and
`Box::AliasR`.

Actions:

- Start from the existing non-template record owner metadata available while
  parsing C-style cast/type-id names; do not recover owner/member identity by
  rendering or splitting `owner::member` text.
- Resolve the member typedef through direct record/member metadata,
  `TypeSpec::record_def`, declaration metadata, or an equivalent structured
  record/member carrier using member `TextId`.
- Keep rendered owner/member spelling only for diagnostics, display, debug
  output, mangling, or final emitted names.
- Cover the `cpp_positive_sema_c_style_cast_member_typedef_ref_alias_basic_cpp`
  failure mode plus nearby same-feature positive cases without adding
  testcase-shaped shortcuts.
- Do not delete the record-body rendered scoped typedef writer in this packet
  unless the structured C-style cast/type-id consumer is first proven.

Completion check:

- C-style cast/type-id parsing for non-template record-body member typedefs no
  longer depends on the rendered generic typedef table for `owner::member`
  entries.
- `Box::AliasL` / `Box::AliasR` style member typedef references resolve
  through structured record/member metadata.
- No helper accepting rendered qualified text, `std::string`,
  `std::string_view`, or fallback spelling is introduced for this semantic
  lookup.
- Narrow parser/Sema tests and a fresh build pass, with fresh canonical
  `test_after.log`, are produced by the executor.

### Step 2.4.4.4: Narrow The Non-Template Member-Typedef Mirror

Goal: delete the obsolete public rendered member-typedef writer and stop
ordinary non-template record-body typedef publication from using generic
rendered `owner::member` storage.

Primary target: `register_struct_member_typedef_binding(owner, member, type)`,
its private rendered-key builder, and non-template record-body member typedef
publication.

Actions:

- Delete the public rendered writer/helper and its private rendered-key
  builder when ordinary record-body member typedefs already publish through a
  structured record/member key.
- Stop non-template record-body member typedef publication from creating
  generic rendered `owner::member` storage.
- Keep any dependent/template compatibility bridge only as an explicitly named
  blocker, not as completion of mirror deletion.
- Search for helper-only renames, wrappers around rendered lookup, fallback
  spelling parameters, local rendered-string reconstruction, and
  `std::string_view` qualified lookup APIs introduced during Step 2.4.

Completion check:

- The obsolete public rendered member-typedef writer/helper is deleted.
- Ordinary non-template record-body member typedefs no longer create generic
  rendered `owner::member` semantic storage.
- Any retained dependent/template compatibility bridge is recorded as the next
  structured metadata target and is not treated as final mirror deletion.
- No helper-only rename, wrapper-only packet, or moved rendered-key
  reconstruction is counted as removal progress.
- Narrow parser tests and a fresh build pass, with fresh canonical
  `test_after.log`, are produced by the executor.

### Step 2.4.4.5A.1: Construct Alias-Template Member-Typedef Carrier Before TypeSpec Flattening

Goal: make alias templates for `typename Owner<Args>::member` preserve
structured owner/member metadata at the parser production site, before that
identity is flattened into rendered or deferred `TypeSpec` fields.

Primary target: the parser alias-template RHS path that recognizes
`typename Owner<Args>::member` and fills `ParserAliasTemplateInfo` or an
equivalent parser/Sema-owned alias-template metadata surface.

Actions:

- Add a first-class alias-template member-typedef carrier for
  `typename Owner<Args>::member` containing the owner `QualifiedNameKey`, the
  parsed/substitutable argument refs or keys, and the member `TextId`.
- Populate the carrier from parser structures while the alias RHS is being
  parsed, before constructing or copying any flattened `TypeSpec` spelling
  fields.
- Preserve the exact owner `QualifiedNameKey` already known to the parser; do
  not render the owner and recover it through `qualified_name_from_text`,
  `tpl_struct_origin`, `TypeSpec::tag`, `qualified_alias_name`, or
  `debug_text`.
- Preserve member identity as `TextId` from the parsed member token; do not
  recover it from `deferred_member_type_name` or a split rendered
  `Owner::member` spelling.
- Preserve argument identity as structured template-argument refs/keys from
  the parser path; do not synthesize the normal route by reparsing saved token
  text or `TemplateArgRef::debug_text`.
- Keep rendered owner/member spelling only for diagnostics, display, debug
  output, mangling, or final emitted text.
- Do not replace the missing carrier with a helper that renders, splits, or
  reparses `Owner::member`, rendered `mangled` spelling, `std::string`,
  `std::string_view`, or fallback spelling.
- Treat a qualified alias path such as `ns::alias_t<...>` as structured only
  if it carries the qualified alias `QualifiedNameKey` or equivalent parser
  key directly; a rendered qualified alias name relay is not progress.

Completion check:

- `ParserAliasTemplateInfo` or an equivalent parser/Sema-owned alias-template
  carrier preserves structured owner `QualifiedNameKey`, argument refs/keys,
  and member `TextId` for dependent member typedef aliases.
- The normal producer path does not seed or recover that carrier from
  rendered/deferred `TypeSpec` fields including `tpl_struct_origin`,
  `deferred_member_type_name`, `TypeSpec::tag`, `qualified_name_from_text`,
  `qualified_alias_name`, or `debug_text`.
- No narrow local alias-of-alias parser duplicates type parsing or substitutes
  template arguments by concatenating token spelling.
- Focused parser/Sema tests prove the carrier survives a drifted rendered or
  deferred spelling disagreement.
- Narrow parser/Sema tests and a fresh build pass, with fresh canonical
  `test_after.log`, are produced by the executor.

### Step 2.4.4.5A.2: Resolve Alias Instantiation Through The Structured Carrier

Goal: make alias instantiation use the carrier from Step 2.4.4.5A.1 to resolve
template primary/specialization member typedefs before any compatibility
bridge is consulted.

Primary target: the parser/Sema alias-template instantiation path that
currently carries only `TypeSpec aliased_type` for dependent member typedef
aliases and later relies on rendered/deferred owner/member fields.

Actions:

- Thread the Step 2.4.4.5A.1 carrier through alias instantiation without
  converting the owner, member, or arguments to rendered spelling and back.
- Resolve `typename Owner<Args>::member` by selecting the template primary or
  specialization from the carrier's owner key and argument refs/keys, then
  finding the member typedef by member `TextId`.
- Keep any existing rendered/deferred `TypeSpec` bridge reachable only as a
  temporary fallback for behavior preservation in this step; do not use it as
  the success criterion.
- Delete or quarantine normal-route calls that infer alias member typedef
  identity from `tpl_struct_origin`, `deferred_member_type_name`,
  `TypeSpec::tag`, `qualified_name_from_text`, `qualified_alias_name`, or
  `debug_text`.
- Do not introduce a narrow alias-of-alias token parser in `parse_top_level` or
  another local parser block to compensate for missing structured carrier data.
- Add or keep focused tests where a structured carrier resolves the alias even
  when rendered/deferred spelling is stale or intentionally disagrees.

Completion check:

- Alias instantiation can resolve `typename Owner<Args>::member` through that
  structured carrier for template primary/specialization member typedefs.
- Qualified alias-template cases do not hand off through rendered qualified
  alias spelling before reaching the carrier lookup.
- The normal structured route does not depend on `TemplateArgRef::debug_text`
  parsing or on reconstructing a `QualifiedNameKey` from text.
- The four known bridge-deletion regressions are examined as same-feature
  coverage, not solved through named-test shortcuts:
  `cpp_positive_sema_eastl_slice7_piecewise_ctor_parse_cpp`,
  `cpp_positive_sema_step3_timeout_probe_baseline_parse_cpp`,
  `cpp_positive_sema_tuple_element_alias_mix_parse_cpp`, and
  `cpp_positive_sema_template_variable_alias_member_typedef_runtime_cpp`.
- Narrow parser/Sema tests and a fresh build pass, with fresh canonical
  `test_after.log`, are produced by the executor.

### Step 2.4.4.5A.3: Review Alias-Template Carrier Route Before Bridge Deletion

Goal: prove the Step 2.4.4.5A route is structurally ready for Step 2.4.4.5B
and does not repeat the rejected rendered/deferred TypeSpec recovery path.

Primary target: the complete Step 2.4.4.5A diff and proof logs.

Actions:

- Review the carrier producer and consumer against
  `review/step2_4_5a_alias_template_carrier_review.md`.
- Confirm carrier data is constructed before rendered/deferred `TypeSpec`
  flattening and then passed through instantiation as structured metadata.
- Confirm retained rendered strings are diagnostic/display/debug/mangling/final
  output only, or are explicitly marked as temporary compatibility fallback
  that Step 2.4.4.5B owns deleting.
- Confirm tests include same-feature disagreement coverage for direct alias,
  qualified alias, and alias-of-alias member-typedef routes.
- If the review still finds string-derived carrier seeding, rendered qualified
  alias relay, debug-text argument parsing, or a narrow local alias-of-alias
  parser, rewrite or split this route again before Step 2.4.4.5B.

Completion check:

- Reviewer/supervisor accepts Step 2.4.4.5A as structured metadata progress,
  not as rendered-string rewrapping.
- `todo.md` records which compatibility bridge paths remain for
  Step 2.4.4.5B.
- No Step 2.4.4.5B bridge deletion is attempted until this review checkpoint is
  satisfied.
- Fresh narrow proof remains available in canonical `test_after.log`.

### Step 2.4.4.5B: Replace The Dependent/Template Member-Typedef Bridge

Goal: delete or structurally bypass the remaining dependent/template
member-typedef compatibility bridge after alias-template member-typedef
metadata is available.

Primary target: the record-body dependent/template compatibility bridge that
still publishes member typedefs through rendered scoped storage while template
primary/specialization member typedef metadata is being completed.

Actions:

- Re-attempt deletion of the dependent/template compatibility bridge after
  Steps 2.4.4.5A.1 through 2.4.4.5A.3 land and pass review.
- Route all remaining template primary, specialization, and alias
  member-typedef readers through structured parser/Sema metadata.
- If another missing carrier is discovered, record that exact carrier as a
  blocker or separate metadata idea before any final mirror deletion is
  attempted.
- Do not replace the bridge with a helper that renders, splits, or reparses
  `owner::member` text, rendered `mangled` spelling, `std::string`,
  `std::string_view`, or fallback spelling.
- Keep rendered template/member spelling only for diagnostics, display, debug
  output, mangling, or final emitted text.

Completion check:

- The dependent/template member-typedef path no longer uses rendered scoped
  storage as semantic authority, or any remaining missing structured carrier is
  represented as an explicit blocker.
- Template primary/specialization member typedef lookup has a structured
  metadata route when parser/Sema has enough ownership to provide one.
- The previously regressed template alias/member-typedef tests are covered
  without adding testcase-shaped shortcuts.
- Narrow parser/Sema tests and a fresh build pass, with fresh canonical
  `test_after.log`, are produced by the executor.

### Step 2.4.4.5B.1: Review Failed Record-Scope Carrier Route

Goal: decide the next structured carrier before any more implementation,
because the first record-scope `using name = typename Owner<Args>::member;`
carrier attempt was reverted as insufficient.

Primary target: the retained
`apply_alias_template_member_typedef_compat_type` projector, the reverted
Step 2.4.4.5B.1 attempt facts in `todo.md`, and the parser path sampled by
`cpp_positive_sema_iterator_concepts_following_hash_base_parse_cpp` and
`cpp_positive_sema_stl_iterator_then_max_size_type_parse_cpp`.

Actions:

- Review the still-live path through `try_parse_record_using_member`,
  `parse_type_name`, `parse_dependent_typename_specifier`, and
  `apply_alias_template_member_typedef_compat_type`.
- Identify whether the missing structured carrier is record-scope using-alias
  RHS metadata, template/dependent record member typedef availability from
  Step 2.4.4.5C, or another parser/Sema-owned carrier.
- Preserve accepted Step 2.4.4.5B partial results: the alias-template context
  fallback and dependent rendered/deferred `TypeSpec` projection in `base.cpp`
  stay deleted.
- Record explicitly that the reverted Step 2.4.4.5B.1 implementation attempt
  is not accepted implementation progress and does not make projector deletion
  safer.
- Do not resume carrier implementation until this review names the exact next
  carrier and the proof command for the executor packet.
- Do not rewrite expectations, mark fixtures unsupported, add named-test
  shortcuts, or replace the projector with another rendered/deferred
  `TypeSpec`, `std::string`, `std::string_view`, `debug_text`, or split
  `Owner::member` route.

Completion check:

- The next packet is one exact implementation route, not "try record-scope
  carrier again".
- The review explains why the two timeout fixtures still need
  `apply_alias_template_member_typedef_compat_type` after the failed
  record-scope attempt.
- Any decision to proceed to Step 2.4.4.5B.2, jump to Step 2.4.4.5C, or split
  again is written into `todo.md` before implementation resumes.
- No implementation progress is claimed from the reverted Step 2.4.4.5B.1
  attempt.

### Step 2.4.4.5B.2: Implement Reviewed Record-Scope Or Adjacent Carrier

Goal: implement only the structured carrier selected by Step 2.4.4.5B.1, so
`apply_alias_template_member_typedef_compat_type` can be retried against a
known missing metadata boundary.

Primary target: the parser/Sema carrier named by Step 2.4.4.5B.1.

Actions:

- Thread the reviewed carrier through parser/Sema metadata without seeding it
  from rendered/deferred `TypeSpec` fields, `tpl_struct_origin`,
  `deferred_member_type_name`, `qualified_name_from_text`,
  `qualified_alias_name`, `debug_text`, or split rendered `Owner::member`
  spelling.
- Preserve owner identity as direct record/template owner metadata,
  `QualifiedNameKey`, namespace-aware key, or other reviewed domain key.
- Preserve member identity as `TextId` from the parsed member token or reviewed
  structured metadata source.
- Keep rendered owner/member spelling only for diagnostics, display, debug
  output, mangling, or final emitted text.
- Preserve accepted Step 2.4.4.5B partial results: the alias-template context
  fallback and dependent rendered/deferred `TypeSpec` projection in `base.cpp`
  stay deleted.
- Stop and update `todo.md` instead of broadening the packet if the reviewed
  carrier is insufficient for the two timeout fixtures.

Completion check:

- The reviewed carrier covers the two timeout fixtures without introducing a
  replacement rendered/deferred lookup route.
- No local rendered-key reconstruction, helper-only rename, expectation
  downgrade, or named-test shortcut is counted as progress.
- Narrow parser/Sema tests and a fresh build pass, with fresh canonical
  `test_after.log`, are produced by the executor.

### Step 2.4.4.5B.3: Delete The Using-Alias Projector

Goal: delete `apply_alias_template_member_typedef_compat_type` only after the
reviewed structured carrier covers its remaining parser/Sema behavior.

Primary target: `apply_alias_template_member_typedef_compat_type` and its
remaining call sites.

Actions:

- Re-attempt deletion of `apply_alias_template_member_typedef_compat_type`
  after Step 2.4.4.5B.2 passes with the two timeout fixtures.
- Confirm no remaining parser/Sema reader requires a deferred member
  `TypeSpec` handoff to recover `Owner<Args>::member` identity.
- If another missing carrier appears, record that exact carrier in `todo.md`
  before proceeding to Step 2.4.4.6.

Completion check:

- `apply_alias_template_member_typedef_compat_type` is deleted, or the exact
  remaining structured carrier blocker is recorded before Step 2.4.4.6.
- The two timeout fixtures and the broader `cpp_positive_sema_` subset pass
  without the projector.
- No local rendered-key reconstruction, helper-only rename, or named-test
  shortcut is counted as progress.
- A fresh build and narrow parser/Sema proof are recorded in canonical
  `test_after.log`.

### Step 2.4.4.5C: Add Dependent/Template Record Member-Typedef Carrier

Goal: replace the remaining record-body bridge that publishes
template/dependent record member typedefs as rendered `source_tag::member`
typedef bindings with a parser/Sema structured carrier.

Primary target: `register_record_member_typedef_bindings()` in
`src/frontend/parser/impl/types/struct.cpp` and the parser route exercised by
`cpp_positive_sema_eastl_slice7_piecewise_ctor_parse_cpp`,
`cpp_positive_sema_step3_timeout_probe_baseline_parse_cpp`, and
`cpp_positive_sema_tuple_element_alias_mix_parse_cpp`.

Actions:

- Identify the structured owner identity available while record member
  typedefs are registered for template/dependent records; use direct record,
  template owner, namespace-aware, or dependent-owner metadata instead of
  rendered `source_tag` spelling.
- Add or thread a parser/Sema-owned carrier for template/dependent record
  member typedef availability keyed by that owner identity plus member
  `TextId`.
- Route the three timed-out positive parse fixtures through the structured
  carrier before consulting the rendered `source_tag::member` publication.
- Treat the existing concrete-instantiation carrier
  `template_instantiation_member_typedefs_by_key` and alias-template
  `ParserAliasTemplateInfo::member_typedef` as already insufficient for this
  route; do not retry deletion by assuming those carriers cover it.
- Do not replace the bridge with a helper that renders, splits, or reparses
  `owner::member` text, rendered `source_tag` spelling, `std::string`,
  `std::string_view`, or fallback spelling.
- Keep rendered owner/member spelling only for diagnostics, display, debug
  output, mangling, or final emitted text.

Completion check:

- Template/dependent record member typedef availability has a structured
  parser/Sema route that covers the three timed-out positive parse fixtures
  without relying on rendered `source_tag::member` typedef binding.
- `register_record_member_typedef_bindings()` no longer needs to publish
  semantic typedef bindings through rendered `source_tag::member`, or any
  remaining uncovered reader is recorded as a new explicit blocker before
  Step 2.4.4.6.
- No local rendered-key reconstruction, helper-only rename, or named-test
  shortcut is counted as progress.
- Narrow parser/Sema tests and a fresh build pass, with fresh canonical
  `test_after.log`, are produced by the executor.

### Step 2.4.4.6: Delete Or Park The Remaining Member-Typedef Mirror

Goal: delete the live `owner::member` typedef mirror, or park only a
non-semantic compatibility cache, after the dependent/template bridge is
converted or recorded as a blocker.

Primary target: any remaining lookup path backed by rendered scoped
member-typedef storage.

Actions:

- Confirm Steps 2.4.4.1 through 2.4.4.5C have removed, converted, or blocked
  every semantic writer/reader that depended on rendered scoped storage.
- Delete the mirror writer and storage only if no semantic consumer remains.
- If a narrow compatibility cache must remain, make its non-semantic purpose
  explicit and ensure it cannot decide parser/Sema semantic identity.
- Represent any remaining metadata blockers as separate open ideas when they
  are outside the current parser/Sema plan.

Completion check:

- No covered parser type/tag/member route uses rendered scoped spelling as
  alternate semantic authority after structured metadata exists.
- No helper-only rename, wrapper-only packet, or moved rendered-key
  reconstruction is counted as removal progress.
- Any remaining metadata blockers are represented as separate open ideas.
- Narrow parser tests and a fresh build pass, with fresh canonical
  `test_after.log`, are produced by the executor.

### Step 2.5: Preserve Parser Const-Int Boundary And HIR Blocker

Goal: keep parser-owned const-int lookup on structured maps while preventing
the Step 2 parser route from absorbing HIR `NttpBindings` carrier migration.

Primary target: parser const-int callers and the parked HIR blocker recorded in
commit `28c1e5c5`.

Actions:

- Verify parser-owned named constants continue to flow through `TextId` or
  structured maps where available.
- Keep the rendered-name `eval_const_int` compatibility overload only for
  callers whose metadata carrier is outside parser/Sema ownership.
- Do not edit `src/frontend/hir` as part of this plan. If the supervisor wants
  to delete the overload, switch to the HIR metadata idea first.
- Keep `todo.md` watchouts explicit when this blocker affects packet choice.

Completion check:

- Parser-owned const-int lookup does not regress to rendered-name authority.
- The HIR `NttpBindings` blocker remains visible in lifecycle state and is not
  silently treated as completed parser work.
- Any code-changing packet has fresh build proof and fresh canonical
  `test_after.log`; todo-only blocker bookkeeping does not claim a code proof.

### Step 3.1: Add Consteval Local And TypeSpec Metadata Producers

Goal: make consteval value/type lookup metadata complete enough that rendered
same-spelling compatibility can be deleted instead of deciding semantic lookup
after structured or `TextId` misses.

Primary target: consteval interpreter value/type metadata producers for
parameters, loop locals, synthetic locals, local declarations, and `TypeSpec`
tag/name carriers.

Actions:

- Read `review/step3_consteval_value_type_review.md` before delegating code for
  this step.
- Inspect the producers that should populate consteval local, loop, parameter,
  and synthetic-local structured metadata before `ConstEvalEnv::lookup(const
  Node*)` runs.
- Inspect the producers that should give consteval `TypeSpec` lookup intrinsic
  identifier metadata instead of relying on rendered-name-to-metadata mirror
  maps.
- Add or repair parser/Sema-owned metadata producers where the owner and
  consumer contract is clear.
- For consteval function-body local/parameter/loop value lookup, expose the
  authoritative structured or `TextId` local-binding miss from producer-backed
  metadata itself. Do not derive producer completeness by consulting rendered
  local-binding maps.
- If a required producer lives outside parser/Sema ownership or has no clear
  source carrier, record that exact blocker in `todo.md` at the lowest correct
  layer before attempting another lookup-deletion packet.
- Do not claim the previous consteval value/type fallback work as closed while
  same-spelling rendered fallback can still decide lookup after populated
  metadata misses.
- Do not delete rendered compatibility in this step unless the matching
  producer completeness is proven for the route being deleted.

Completion check:

- `todo.md` names the exact consteval metadata producer repaired or the exact
  blocker parked.
- Consteval local/loop/parameter/synthetic-local lookup has enough structured
  or `TextId` metadata to make metadata misses authoritative for the covered
  route, or the missing producer is explicitly parked.
- Consteval `TypeSpec` lookup no longer depends on rendered-name-to-metadata
  mirrors for the covered route, or that dependency is explicitly parked.
- Narrow Sema/frontend tests and a fresh build pass, with fresh canonical
  `test_after.log`, are produced by the executor for code-changing packets.

### Step 3.2: Delete Consteval Rendered Compatibility After Metadata Completion

Goal: remove consteval value/type rendered-name fallback only after Step 3.1
has repaired or explicitly parked the matching metadata producer gap.

Primary target: `ConstEvalEnv::lookup(const Node*)`, consteval type binding
lookup helpers, and consteval function/value/type lookup APIs that still accept
or consult rendered spelling.

Actions:

- Block rendered fallback after any populated structured or `TextId` metadata
  miss, including same-spelling local names.
- Do not use a rendered-name local-binding probe, rendered local map, or helper
  equivalent to decide whether a structured or `TextId` miss is authoritative.
  If the metadata cannot identify that covered miss without rendered lookup,
  return to Step 3.1 producer repair or park the gap before deleting fallback.
- Delete or collapse consteval semantic lookup APIs that take `std::string`,
  `std::string_view`, rendered names, or fallback spelling once the route has a
  structured/domain key.
- Keep no-metadata rendered compatibility only when `todo.md` names the missing
  producer and the fallback is explicitly out of scope for the packet.
- Keep rendered strings only for diagnostics, display, debug output, mangling,
  ABI/link spelling, or final emitted text.
- Add focused same-feature tests proving metadata miss rejection and no stale
  same-spelling rendered recovery for the covered route.

Completion check:

- Covered consteval value/type paths no longer use rendered spelling as
  semantic authority after structured or `TextId` metadata exists and misses.
- Covered consteval semantic lookup APIs no longer accept string/string_view or
  fallback spelling parameters.
- Any remaining no-metadata compatibility route is recorded as a blocker or
  separate metadata idea before Step 3.2 is treated as complete.
- Narrow Sema/frontend tests and a fresh build pass, with fresh canonical
  `test_after.log`, are produced by the executor.

### Step 3.3: Remove Remaining Sema Owner/Member/Static Rendered Routes

Goal: Sema owner/member/static lookup must not consult rendered names after
owner keys, AST nodes, declarations, or structured maps are available.

Primary target: Sema owner/member/static lookup paths outside the consteval
value/type metadata-producer route.

Actions:

- Inspect structured owner key construction and lookup call sites.
- Convert rendered-name probes to structured owner/member/static lookups where
  producer metadata already exists.
- Prioritize remaining Sema-owned `fallback`/`legacy` routes that can be
  deleted with existing metadata, including `lookup_struct_static_member_*`,
  instance-field legacy probes, owner-by-rendered-name fallback, and rendered
  global/enum compatibility indexes. Park only the routes whose missing
  producer is named in `todo.md` or a separate idea.
- Delete Sema semantic lookup APIs that take `std::string`, `std::string_view`,
  or fallback spelling arguments.
- Collapse Sema overload families so each semantic lookup uses a structured
  owner, declaration, `TextId`, or domain key route.
- Remove Sema semantic helpers whose `fallback` or `legacy` names preserve
  rendered-spelling compatibility.
- Add focused Sema tests for same-feature structured-vs-rendered disagreement.

Completion check:

- Covered Sema paths use structured semantic authority only.
- Covered Sema semantic lookup APIs no longer accept string/string_view or
  fallback spelling parameters.
- Covered Sema overload families are collapsed to structured/domain-key APIs.
- Any missing mirror or producer metadata is recorded as a new open idea.
- Narrow Sema tests and a fresh build pass, with fresh canonical
  `test_after.log`, are produced by the executor.

### Step 4: Repair Parser-to-Sema Metadata Handoff Gaps

Goal: prevent parser-to-Sema handoff from dropping metadata and forcing later
string rediscovery.

Primary target: AST and semantic handoff fields carrying `TextId`, namespace
context, record identity, `TypeSpec::record_def`, declarations, and owner keys.

Actions:

- Trace handoff for typedefs, values, records, templates, NTTP defaults, known
  functions, members, static members, and consteval references.
- Preserve existing structured metadata through parser-to-Sema boundaries.
- Add missing fields only when the producer and consumer contract is clear and
  scoped to parser/Sema.
- Preserve `TextId` only as text identity. When lookup requires namespace,
  owner, declaration, record, template, or value meaning, hand off the domain
  carrier instead of adding fallback spelling.
- Create separate open ideas for larger cross-module carriers.
- Add tests where handoff preserves structured identity despite drifted
  rendered spelling.

Completion check:

- Covered handoff paths preserve available structured metadata.
- New metadata blockers, if any, are represented as separate open ideas.
- Focused frontend tests and a fresh build pass.

### Step 5: Audit For Rename-Only Or Wrapper-Only Work

Goal: ensure the plan did not replace old rendered-string authority with a
newly named equivalent.

Actions:

- Inspect the diff for helper-only renames, comment-only classification, or
  string wrappers still reachable from semantic lookup call sites.
- Inspect the diff for string/string_view semantic lookup parameters,
  `TextId` plus fallback spelling APIs, parallel string and structured
  overloads, and semantic names containing `fallback` or `legacy`.
- Search parser/Sema for rendered-name semantic probes introduced or retained
  by this work.
- Reject any packet whose main effect is rewording rather than removal.
- Confirm tests do not rely on rendered-name authority after structured
  metadata is available.

Completion check:

- No helper-only rename packet is counted as progress.
- No covered parser/Sema semantic lookup API still accepts string/string_view,
  fallback spelling, or both string and structured overloads.
- Remaining parser/Sema string uses are non-semantic output or source spelling.
- A broader frontend validation checkpoint passes.

### Step 6: Final Frontend Coverage And Regression Check

Goal: prove the source idea acceptance criteria without relying on one named
testcase.

Actions:

- Run focused parser/Sema tests for covered structured-vs-rendered
  disagreement paths.
- Run a broader frontend validation command selected by the supervisor, such as
  a frontend-focused `ctest` subset or matching `c4c-regression-guard` scope.
- Inspect the diff for unsupported expectation downgrades, narrow named-case
  shortcuts, retained semantic string authority, fallback/legacy compatibility
  lookup helpers, or string/structured overload families.

Completion check:

- Acceptance criteria from the source idea are satisfied.
- `todo.md` records proof commands and results.
- The supervisor has enough evidence to decide whether the source idea can be
  closed or must split remaining metadata blockers into new ideas.
