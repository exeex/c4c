# HIR Legacy String Lookup Removal Convergence Runbook

Status: Active
Source Idea: ideas/open/124_hir_legacy_string_lookup_removal_convergence.md

## Purpose

Converge `src/frontend/hir` away from legacy rendered-string lookup authority.
HIR-owned text lookup should use `TextId` / `TextTable` keys, while semantic
lookup should use declaration IDs, namespace context IDs, `LinkNameId`, typed
records, or other domain-specific structured identity where that metadata is
already available.

## Goal

Classify and reduce HIR string-keyed lookup paths without removing strings that
are required for diagnostics, dumps, mangled/link-visible output, ABI spelling,
or compatibility bridges.

## Core Rule

`TextId` is text identity, not semantic authority. Use `TextId` for interned
text lookup, and use declaration IDs or domain semantic tables for semantic
lookup. Rendered strings may remain as spelling, display, compatibility, or
final-output data only after their role is explicit.

## Read First

- `ideas/open/124_hir_legacy_string_lookup_removal_convergence.md`
- `src/frontend/hir`
- Existing HIR tests that cover module symbols, namespace-qualified symbols,
  templates, consteval/bodyless paths, globals, functions, and link-name-backed
  lookup.

## Current Targets

- HIR-owned `std::unordered_map<std::string, ...>` lookup maps.
- String fallback paths under `src/frontend/hir`.
- Tests that still assert rendered-name precedence where structured identity is
  intended authority.

## Non-Goals

- Parser helper cleanup.
- LIR type-ref authority redesign.
- BIR verifier or printer contract changes except for focused downstream proof
  needed by a HIR-owned slice.
- Broad struct/type identity removal unless a separate idea is opened.
- Removing emitted spelling needed for ABI, diagnostics, dumps, or final output.
- Weakening supported tests, marking covered paths unsupported, or adding
  named-case shortcuts.

## Working Model

- Pure text lookup maps become `TextId` keyed when HIR already has interned
  text.
- Semantic lookup prefers declaration IDs, namespace context IDs, `LinkNameId`,
  typed records, or domain tables before rendered-name fallbacks.
- Rendered strings retained after cleanup must be classified as display,
  diagnostics, dump text, mangled/final spelling, compatibility payload, or an
  unresolved producer/consumer boundary.
- Cross-module failures should be resolved by the intended authority contract.
  If a blocker needs upstream or downstream work outside HIR, record a new idea
  instead of expanding this runbook.

## Execution Rules

- Keep each implementation packet behavior-preserving unless the source idea
  explicitly allows updating stale tests that preserve legacy string authority.
- Do not convert diagnostic, dump, ABI, or final spelling fields into semantic
  authority.
- When both structured identity and drifted rendered spelling exist, structured
  identity wins for covered lookup paths.
- Update tests only when they assert legacy rendered-name authority rather than
  supported semantics.
- Every code-changing packet needs fresh build or compile proof plus a focused
  test subset chosen by the supervisor.
- Escalate to broader HIR or downstream validation when a packet changes shared
  symbol lookup, namespace resolution, function/global lookup, or codegen-facing
  carriers.

## Step 1: Inventory HIR String Lookup Surfaces

Goal: build the working map of HIR string-keyed lookup authority and retained
spelling surfaces.

Primary target: `src/frontend/hir`

Actions:
- Inspect `std::unordered_map<std::string, ...>`, string comparison, rendered
  name fallback, and symbol matching paths under `src/frontend/hir`.
- Group findings into pure text lookup, semantic lookup, diagnostics/display,
  dump/final spelling, compatibility bridge, and unresolved boundary categories.
- Identify the first narrow conversion target where HIR already has stable
  `TextId` or structured semantic metadata.
- Record packet notes in `todo.md`; do not rewrite the source idea for routine
  inventory findings.

Completion check:
- `todo.md` names the first bounded conversion packet and the category of the
  major remaining string surfaces.

## Step 2: Convert Covered Pure Text Lookup Maps To TextId

Goal: move HIR-owned text lookup maps off raw `std::string` keys where the
lookup text is already interned.

Primary target: pure text lookup surfaces found in Step 1.

Actions:
- Replace local string-keyed text lookup authority with `TextId` keys.
- Resolve parser/HIR text through existing `TextTable` or stored text IDs rather
  than re-rendering names for lookup.
- Keep rendered spellings only for diagnostics, dumps, compatibility payloads,
  or final output.
- Add focused tests that prove stale or drifted rendered spelling does not beat
  the covered text-id lookup.

Completion check:
- Covered text lookup paths use `TextId` keys.
- Focused HIR tests pass and prove the drift case for at least one converted
  lookup surface.

## Step 3: Demote Rendered-Name Semantic Lookup For Functions And Globals

Goal: prefer existing semantic identity over rendered names for HIR function
and global lookup paths.

Primary targets: HIR function/global symbol lookup, namespace-qualified lookup,
and link-name-backed carriers where metadata is already present.

Actions:
- Identify function/global lookups where declaration IDs, namespace context IDs,
  `LinkNameId`, typed records, or semantic mirrors already exist.
- Route semantic lookup through those structured identities before rendered
  spelling fallbacks.
- Preserve rendered names as final spelling, diagnostics, dumps, mangling, or
  compatibility data.
- Update tests that protect drifted rendered-name semantic authority.

Completion check:
- Covered function/global paths do not use rendered strings as primary semantic
  lookup authority.
- Focused tests prove structured identity wins when rendered spelling drifts.

## Step 4: Extend Structured Lookup Proof Across HIR Edge Paths

Goal: cover HIR edge paths most likely to expose mixed-contract drift.

Primary targets: namespace-qualified symbols, template-related paths,
consteval/bodyless declarations, and link-name-backed symbol flows.

Actions:
- Select narrow packets from the Step 1 inventory where structured metadata is
  already available.
- Convert or demote lookup authority using the same text-id or semantic-id rule.
- Add mismatch tests for nearby cases, not only a single known failing fixture.
- Split missing metadata propagation into a new open idea if the required data
  is absent.

Completion check:
- Edge-path tests show `TextId` or semantic identity survives stale rendered
  spelling.
- Any out-of-scope metadata gap is represented as a separate idea rather than
  hidden in this plan.

## Step 5: Classify Remaining HIR String Surfaces

Goal: make retained strings visibly non-authoritative or explicitly unresolved.

Primary target: remaining HIR string-keyed or rendered-name surfaces after
covered conversions.

Actions:
- Rename helpers, parameters, comments, or tests where needed to show retained
  string paths are display, diagnostic, dump, final-spelling, compatibility, or
  unresolved-boundary surfaces.
- Avoid deleting strings needed for ABI, dumps, diagnostics, or codegen-facing
  spelling.
- Keep public or downstream bridge surfaces classified if they cannot be safely
  converted in this HIR runbook.

Completion check:
- Remaining HIR string lookup surfaces are classified by purpose in code,
  tests, or `todo.md`.
- No covered string map remains as silent semantic authority.

## Step 6: Reprove HIR Cleanup And Decide Closure Or Split

Goal: validate the completed HIR cleanup route and decide whether the source
idea is complete or needs a follow-up split.

Actions:
- Run the supervisor-selected broader HIR proof, escalating to downstream
  LIR/BIR tests when the changed carriers cross module boundaries.
- Review the diff for testcase-overfit, unsupported downgrades, and retained
  rendered-name semantic authority.
- If HIR scope is complete, prepare for lifecycle close.
- If a separate unresolved metadata or downstream contract issue remains, write
  or request a new idea under `ideas/open/` and keep this source idea focused.

Completion check:
- Regression proof is green for the selected close scope.
- The plan owner can decide whether to close this source idea or split a
  follow-up without re-inventorying the entire HIR module.
