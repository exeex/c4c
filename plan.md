# Parser/Sema Legacy String Lookup Resweep Runbook

Status: Active
Source Idea: ideas/open/139_parser_sema_legacy_string_lookup_resweep.md

## Purpose

Run a second parser and Sema cleanup pass for legacy rendered-string lookup
authority after the prior frontend cleanup series.

Goal: make parser and Sema semantic lookup structured-primary wherever
`TextId`, namespace context ids, `QualifiedNameKey`, direct AST links,
`TypeSpec::record_def`, or Sema owner keys already exist.

## Core Rule

Rendered strings may remain as source spelling, diagnostics, display/final
rendering, compatibility input, or explicitly classified TextId-less fallback.
They must not silently decide parser or Sema semantic lookup when structured
metadata is available.

## Read First

- `ideas/open/139_parser_sema_legacy_string_lookup_resweep.md`
- `src/frontend/parser`
- `src/frontend/sema`
- Parser typedef, value, tag, template, NTTP-default, and known-function lookup
  paths
- Sema owner/member/static lookup paths
- Parser-to-Sema AST and semantic handoff surfaces

## Current Targets

- Parser lookup maps and rendered-name fallback helpers
- Parser compatibility mirror reads
- Sema structured owner/member/static lookup routes
- Parser-to-Sema metadata handoff for text identity, namespace context, record
  identity, and owner keys
- Focused frontend tests for structured-vs-rendered disagreement cases

## Non-Goals

- Do not change HIR, LIR, BIR, or backend implementation except to record a
  separate metadata handoff blocker under `ideas/open/`.
- Do not remove diagnostic, source-spelling, dump, or final emitted text
  strings.
- Do not weaken supported behavior, mark tests unsupported, or use
  testcase-shaped shortcuts.
- Do not treat `TextId` alone as semantic authority when a domain key or
  direct semantic carrier exists.

## Working Model

Parser and Sema lookup should follow this order:

1. Use direct AST links, Sema structured owner keys, `QualifiedNameKey`,
   namespace-aware keys, or other domain semantic carriers when available.
2. Use `TextId` as stable text identity where the lookup domain makes that
   correct.
3. Use rendered strings only for spelling/display, producer-boundary
   compatibility, or explicitly classified missing-metadata fallback.
4. When a downstream cleanup needs metadata that parser/Sema cannot honestly
   provide yet, split that bridge into a new open idea instead of expanding
   this runbook.

## Execution Rules

- Start with inventory and classification before deleting or converting
  fallbacks.
- Keep each implementation packet narrow and prove it with a fresh build or
  focused frontend test subset.
- Prefer semantic lowering and structured key propagation over named-case
  matching.
- Tests should cover same-feature drifted-string behavior, not only one narrow
  testcase.
- Update `todo.md` for packet progress; rewrite this runbook only when the
  route contract changes.

## Steps

### Step 1: Inventory Parser and Sema String Lookup Authority

Goal: identify parser and Sema paths where rendered strings still decide
semantic lookup despite available structured metadata.

Primary target: `src/frontend/parser` and `src/frontend/sema`.

Actions:

- Search for `std::string` keyed lookup maps, rendered-name fallback helpers,
  and compatibility mirror reads in parser and Sema.
- Classify each string surface as semantic authority, display/diagnostic,
  final spelling, compatibility input, TextId-less fallback, or local scratch.
- Trace parser typedef, value, tag, template, NTTP-default, and known-function
  lookup paths.
- Trace Sema owner/member/static lookup paths that fall back from structured
  keys to rendered names.
- Identify the smallest first implementation packet that converts or demotes
  one real semantic lookup path.

Completion check:

- `todo.md` records the selected first implementation packet and proof command
  for supervisor delegation.
- The inventory distinguishes structured-primary, legitimate string, and
  suspicious string-authority routes.

### Step 2: Demote Parser Lookup Fallbacks Where Structured Keys Exist

Goal: make parser lookup routes prefer existing structured carriers over
rendered names.

Primary target: parser typedef, value, tag, template, NTTP-default, and
known-function lookup paths with existing structured alternatives.

Actions:

- Route lookups through `TextId`, namespace context ids, `QualifiedNameKey`, or
  direct AST links where those carriers already exist.
- Keep rendered-name mirrors only as compatibility fallback or spelling data.
- Make structured/rendered disagreement observable when practical.
- Add focused parser tests for drifted rendered spelling where structured
  metadata should win.

Completion check:

- Covered parser paths are structured-primary.
- Compatibility fallback behavior remains explicit and tested where retained.
- Narrow parser tests and a fresh build pass.

### Step 3: Tighten Sema Owner, Member, and Static Lookup Authority

Goal: prevent Sema structured owner/member/static lookup from silently falling
back to rendered names when owner keys or direct semantic carriers are present.

Primary target: Sema owner/member/static lookup paths and structured owner key
helpers.

Actions:

- Inspect structured owner key construction and lookup call sites.
- Convert fallback paths to structured-primary lookup where the producer
  already supplies owner or record identity.
- Preserve rendered names for diagnostics and compatibility inputs.
- Add focused Sema tests for same-feature structured-vs-rendered disagreement.

Completion check:

- Covered Sema paths use structured owner/member/static lookup authority.
- Retained rendered-name fallback is classified and guarded.
- Narrow Sema tests and a fresh build pass.

### Step 4: Repair Parser-to-Sema Metadata Handoff Gaps

Goal: prevent parser-to-Sema handoff from dropping metadata and forcing later
rendered-string rediscovery.

Primary target: AST and semantic handoff fields carrying `TextId`, namespace
context, record identity, `TypeSpec::record_def`, and owner keys.

Actions:

- Trace handoff for typedefs, values, records, templates, NTTP defaults,
  known functions, members, and static members.
- Preserve existing structured metadata through parser-to-Sema boundaries.
- If a required cross-module carrier is missing, create a separate
  `ideas/open/` follow-up instead of widening this plan.
- Add tests where handoff preserves structured identity despite drifted
  rendered spelling.

Completion check:

- Covered handoff paths preserve available structured metadata.
- New metadata blockers, if any, are represented as separate open ideas.
- Focused frontend tests and a fresh build pass.

### Step 5: Consolidate Classification and Compatibility Guards

Goal: make retained parser/Sema string surfaces explicit and difficult to
mistake for semantic authority.

Primary target: helper names, comments, fallback wrappers, and tests around
retained string lookup surfaces.

Actions:

- Rename or extract helpers where needed so call sites distinguish structured
  lookup, compatibility fallback, and display/final spelling.
- Remove duplicated ad hoc rendered-name fallback logic from paths that can use
  a central structured-primary helper.
- Keep concise comments only where they prevent future authority drift.
- Confirm tests do not rely on rendered-name authority after structured
  metadata is available.

Completion check:

- Retained parser/Sema string surfaces are classified by role.
- Callers no longer silently prefer rendered strings over available structured
  metadata.
- A broader frontend validation checkpoint passes.

### Step 6: Final Frontend Coverage and Regression Check

Goal: prove the source idea acceptance criteria without relying on one named
testcase.

Actions:

- Run focused parser/Sema tests for covered structured-vs-rendered
  disagreement paths.
- Run a broader frontend validation command selected by the supervisor, such as
  a frontend-focused `ctest` subset or matching `c4c-regression-guard` scope.
- Inspect the diff for unsupported expectation downgrades, narrow named-case
  shortcuts, or unclassified semantic string authority.

Completion check:

- Acceptance criteria from the source idea are satisfied.
- `todo.md` records proof commands and results.
- The supervisor has enough evidence to decide whether the source idea can be
  closed or needs another runbook.
