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
- Parser call sites that render `QualifiedNameKey` and then re-enter
  string lookup
- Sema consteval, owner, member, and static lookup routes that consult rendered
  names after structured keys exist
- Sema semantic lookup overload families that preserve both string and
  structured routes
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

### Step 2.4.4.2: Add Template-Instantiation Member-Typedef Carrier

Goal: make the template-instantiation member-typedef writer use parser-owned
structured storage keyed by concrete template-instantiation identity plus
member text identity.

Primary target: the template-instantiation path in
`src/frontend/parser/impl/types/base.cpp` that currently reaches member typedef
registration through rendered instantiation spelling, plus the minimal parser
metadata/API surface required to store and query:
`TemplateInstantiationKey concrete_owner + TextId member_text_id`.

Actions:

- Add a parser-owned member-typedef carrier/API for concrete template
  instantiations instead of forcing `TemplateInstantiationKey` through
  `QualifiedNameKey` or rendered spelling.
- Reuse the call site's existing structured concrete-instantiation metadata:
  primary template `QualifiedNameKey`, concrete
  `TemplateInstantiationKey::Argument` vector, and direct member name
  `TextId`.
- Convert the template-instantiation writer to register member typedefs
  through the concrete template-instantiation carrier.
- Add reader plumbing only for callers that can query from the same concrete
  template owner plus member `TextId`; do not add a generic rendered
  `owner::member` rediscovery route.
- Treat any need to cross into HIR, LIR, BIR, or backend metadata as a separate
  open idea instead of widening this parser/Sema plan.
- Keep rendered instantiation spelling only for mangling, diagnostics, debug
  output, or final emitted names.
- Add or keep a focused disagreement test proving the writer does not let stale
  rendered scoped typedef storage decide semantic lookup.

Completion check:

- Parser metadata contains a structured storage/API path for
  `TemplateInstantiationKey concrete_owner + TextId member_text_id`; it does
  not alias all specializations of the same primary template.
- The covered writer no longer builds semantic `QualifiedNameKey` or
  owner/member identity by parsing rendered instantiation text or using
  rendered `mangled` as concrete owner identity.
- Any remaining missing metadata outside parser/Sema is represented as a
  separate open blocker idea rather than a new string rediscovery route.
- Narrow parser tests and a fresh build pass, with fresh canonical
  `test_after.log`, are produced by the executor.

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

### Step 2.4.4.5: Replace The Dependent/Template Member-Typedef Bridge

Goal: replace the remaining dependent/template member-typedef compatibility
bridge with structured parser metadata, or record the exact missing carrier as
a blocker before any final mirror deletion is attempted.

Primary target: the record-body dependent/template compatibility bridge that
still publishes member typedefs through rendered scoped storage while template
primary/specialization member typedef metadata is not available early enough.

Actions:

- Trace the dependent/template reader and writer that still require the
  compatibility bridge, including template primary, specialization, and alias
  member-typedef cases.
- Identify the structured carrier needed during record-body finalization before
  post-parse template parameter attachment runs.
- Add or route through structured metadata for template primary/specialization
  member typedef lookup when it is locally available in parser/Sema scope.
- If the carrier belongs outside parser/Sema or cannot be made available in
  this route, record the exact blocker instead of deleting the bridge.
- Do not replace the bridge with a helper that renders, splits, or reparses
  `owner::member` text, rendered `mangled` spelling, `std::string`,
  `std::string_view`, or fallback spelling.
- Keep rendered template/member spelling only for diagnostics, display, debug
  output, mangling, or final emitted text.

Completion check:

- The dependent/template member-typedef path no longer uses rendered scoped
  storage as semantic authority, or the precise missing structured carrier is
  recorded as a blocker.
- Template primary/specialization member typedef lookup has a structured
  metadata route when parser/Sema has enough ownership to provide one.
- The previously regressed template alias/member-typedef tests are covered
  without adding testcase-shaped shortcuts.
- Narrow parser/Sema tests and a fresh build pass, with fresh canonical
  `test_after.log`, are produced by the executor.

### Step 2.4.4.6: Delete Or Park The Remaining Member-Typedef Mirror

Goal: delete the live `owner::member` typedef mirror, or park only a
non-semantic compatibility cache, after the dependent/template bridge is
converted or recorded as a blocker.

Primary target: any remaining lookup path backed by rendered scoped
member-typedef storage.

Actions:

- Confirm Steps 2.4.4.1 through 2.4.4.5 have removed, converted, or blocked
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

### Step 3: Remove Sema Rendered-String Owner And Consteval Lookup Routes

Goal: Sema owner/member/static/consteval lookup must not consult rendered names
after owner keys, AST nodes, declarations, or structured maps are available.

Primary target: Sema owner/member/static lookup paths and consteval lookup
helpers.

Actions:

- Inspect structured owner key construction and lookup call sites.
- Convert rendered-name probes to structured owner/member/static lookups where
  producer metadata already exists.
- Delete Sema semantic lookup APIs that take `std::string`, `std::string_view`,
  or fallback spelling arguments.
- Collapse Sema overload families so each semantic lookup uses a structured
  owner, declaration, `TextId`, or domain key route.
- Remove Sema semantic helpers whose `fallback` or `legacy` names preserve
  rendered-spelling compatibility.
- Remove string recovery from consteval function/value/type lookup where the
  `Node*`, declaration, or structured key is available.
- Add focused Sema tests for same-feature structured-vs-rendered disagreement.

Completion check:

- Covered Sema paths use structured semantic authority only.
- Covered Sema semantic lookup APIs no longer accept string/string_view or
  fallback spelling parameters.
- Covered Sema overload families are collapsed to structured/domain-key APIs.
- Any missing mirror or producer metadata is recorded as a new open idea.
- Narrow Sema tests and a fresh build pass.

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
