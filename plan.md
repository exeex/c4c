# Parser/Sema Rendered String Lookup Removal Runbook

Status: Active
Source Idea: ideas/open/139_parser_sema_rendered_string_lookup_removal.md

## Purpose

Finish the parser/Sema cleanup by deleting rendered-string semantic lookup
routes instead of renaming or reclassifying them.

Goal: parser/Sema semantic lookup must use direct AST links, declaration/owner
objects, namespace context ids, `TextId`, `QualifiedNameKey`,
`TypeSpec::record_def`, or Sema structured owner keys where those carriers are
available.

## Core Rule

If structured metadata is available, rendered strings must not decide parser or
Sema semantic identity. A string-keyed semantic route is acceptable only while
the same packet is replacing it, or while a new metadata idea is being created
because the required carrier does not exist.

## Read First

- `ideas/open/139_parser_sema_rendered_string_lookup_removal.md`
- `src/frontend/parser`
- `src/frontend/sema`
- Parser typedef, value, tag, template, NTTP-default, and known-function lookup
  paths
- Sema owner/member/static/consteval lookup paths
- Parser-to-Sema AST and semantic handoff surfaces

## Current Targets

- Parser string-keyed semantic lookup maps and rendered-name probes
- Parser call sites that render `QualifiedNameKey` and then re-enter
  string lookup
- Sema consteval, owner, member, and static lookup routes that consult rendered
  names after structured keys exist
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

## Working Model

Parser and Sema lookup should follow this order:

1. Use direct AST links, declaration/owner objects, Sema structured owner keys,
   `QualifiedNameKey`, namespace-aware keys, or other domain semantic carriers.
2. Use `TextId` only where text identity is the correct lookup domain.
3. Use rendered strings only for non-semantic output: source spelling,
   diagnostics, display, dumps, final emitted text, and ABI/link spelling.
4. When a semantic lookup cannot be converted because metadata is missing,
   create a new open idea for that producer/consumer contract and remove the
   string rediscovery goal from this packet.

## Execution Rules

- Start each packet by naming the exact rendered-string semantic route being
  removed.
- The packet must either delete that route, replace it with structured
  metadata, or open a metadata blocker idea.
- Do not present renamed helpers or newly labeled string wrappers as progress.
- Tests should cover same-feature drifted-string behavior, not only one narrow
  testcase.
- Update `todo.md` for packet progress; rewrite this runbook only when the
  route contract changes.

## Steps

### Step 1: Inventory Rendered-String Semantic Authority

Goal: identify parser and Sema paths where rendered strings still decide
semantic lookup.

Primary target: `src/frontend/parser` and `src/frontend/sema`.

Actions:

- Search for string-keyed semantic maps, rendered-name probes, and helper paths
  that re-enter lookup through spelling.
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

### Step 2: Remove Parser Rendered-String Semantic Lookup Routes

Goal: parser lookup must not render a structured key and then use the rendered
spelling as semantic authority.

Primary target: parser typedef, value, tag, template, NTTP-default, and
known-function lookup paths with existing structured alternatives.

Actions:

- Convert lookups to `TextId`, namespace context ids, `QualifiedNameKey`,
  direct AST links, or `TypeSpec::record_def` where those carriers already
  exist.
- Delete string re-entry after structured-key misses when the caller supplied a
  valid structured carrier.
- Where deletion exposes missing producer metadata, create a new open idea
  instead of keeping string rediscovery.
- Add focused parser tests for drifted rendered spelling where structured
  metadata must win.

Completion check:

- Covered parser paths no longer use rendered spelling as alternate semantic
  authority.
- New metadata blockers, if any, are represented as separate open ideas.
- Narrow parser tests and a fresh build pass.

### Step 3: Remove Sema Rendered-String Owner And Consteval Lookup Routes

Goal: Sema owner/member/static/consteval lookup must not consult rendered names
after owner keys, AST nodes, declarations, or structured maps are available.

Primary target: Sema owner/member/static lookup paths and consteval lookup
helpers.

Actions:

- Inspect structured owner key construction and lookup call sites.
- Convert rendered-name probes to structured owner/member/static lookups where
  producer metadata already exists.
- Remove string recovery from consteval function/value/type lookup where the
  `Node*`, declaration, or structured key is available.
- Add focused Sema tests for same-feature structured-vs-rendered disagreement.

Completion check:

- Covered Sema paths use structured semantic authority only.
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
- Search parser/Sema for rendered-name semantic probes introduced or retained
  by this work.
- Reject any packet whose main effect is rewording rather than removal.
- Confirm tests do not rely on rendered-name authority after structured
  metadata is available.

Completion check:

- No helper-only rename packet is counted as progress.
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
  shortcuts, or retained semantic string authority.

Completion check:

- Acceptance criteria from the source idea are satisfied.
- `todo.md` records proof commands and results.
- The supervisor has enough evidence to decide whether the source idea can be
  closed or must split remaining metadata blockers into new ideas.
