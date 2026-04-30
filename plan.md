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

### Step 2.2: Remove Parser Declarator And Known-Function Rendered Recovery

Goal: parser declarator, value, and known-function lookup must not recover
semantic identity by parsing or searching rendered qualified names after token
or namespace metadata exists.

Primary target: parser declaration and statement paths that register or query
values, function declarations, known functions, and qualified declarator names.

Actions:

- Inspect qualified declarator parsing, known-function registration, static
  member/value lookup, and expression owner lookup call sites.
- Convert lookup authority to `TextId`, namespace context ids,
  `QualifiedNameKey`, direct AST links, or declaration objects where available.
- Delete string re-entry after a structured key, declaration name `TextId`, or
  namespace-aware key was supplied.
- Collapse parser overload families that keep both rendered-name and
  structured lookup routes for the same value/function query.
- Keep rendered names only for source spelling, diagnostics, mangling, ABI/link
  names, or final emitted text.
- Add focused parser tests for drifted rendered qualified names in the covered
  value/function route.

Completion check:

- Covered declarator/value/known-function paths no longer use rendered
  spelling as alternate semantic authority.
- Covered APIs no longer accept string/string_view or fallback spelling
  parameters for semantic lookup.
- Tests prove structured metadata wins over drifted rendered spelling.
- Narrow parser tests and a fresh build pass, with fresh canonical
  `test_after.log`, are produced by the executor.

### Step 2.3: Audit Parser Type, Tag, And Member-Typedef Routes

Goal: verify the already-converted parser typedef, tag, record type-head, and
member-typedef routes did not leave a reachable rendered-string authority
behind a renamed helper or compatibility wrapper.

Primary target: parser type parsing, record lookup, structured typedef
bindings, `TypeSpec::record_def`, member typedef arrays, and tag lookup routes.

Actions:

- Inspect the completed Step 2 parser changes for helper-only renames,
  comment-only classification, or new wrappers around rendered lookup.
- Search for string/string_view semantic lookup parameters, fallback spelling
  parameters, `resolved_tag + "::" + member` lookup, and
  `struct_tag_def_map.find(rendered-name)` after a structured miss.
- Confirm `TextId` is used only for text identity and richer domain keys are
  used when namespace, owner, record, template, or declaration identity matters.
- Add or preserve focused parser disagreement tests for typedef, tag, record
  type-head, and member-typedef behavior.
- If the audit finds missing producer metadata outside parser/Sema ownership,
  record a metadata idea instead of restoring rendered lookup.

Completion check:

- No covered parser type/tag/member route uses rendered spelling as alternate
  semantic authority after structured metadata exists.
- No helper-only rename or wrapper-only packet is counted as removal progress.
- Any new metadata blockers are represented as separate open ideas.
- Narrow parser tests and a fresh build pass, with fresh canonical
  `test_after.log`, are produced by the executor.

### Step 2.4: Preserve Parser Const-Int Boundary And HIR Blocker

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
