# BIR Legacy String Lookup Removal Convergence Runbook

Status: Active
Source Idea: ideas/open/126_bir_legacy_string_lookup_removal_convergence.md

## Purpose

Converge `src/backend/bir` away from legacy raw-string lookup authority after
the first BIR string cleanup. BIR should use interned text identity for text
lookup and structured semantic identity for semantic lookup.

## Goal

Classify and reduce BIR string-keyed lookup paths while preserving strings
needed for display, selectors, dumps, diagnostics, final spelling,
compatibility payloads, or unresolved upstream boundaries.

## Core Rule

`TextId` is text identity, not semantic authority. Use `TextId` for interned
text lookup, and use typed BIR records, `LinkNameId`, block label IDs,
structured type IDs, or domain semantic tables for semantic lookup when those
carriers already exist. Raw strings may remain only after their role is
explicit.

## Read First

- `ideas/open/126_bir_legacy_string_lookup_removal_convergence.md`
- `src/backend/bir`
- BIR verifier and printer tests.
- LIR-to-BIR route tests that cover direct calls, globals, externs, labels,
  structured types, and mismatch behavior.
- Recent BIR LinkNameId cleanup history when deciding whether a string surface
  is display spelling, compatibility payload, or unresolved producer data.

## Current Targets

- BIR-owned `std::unordered_map<std::string, ...>` lookup maps.
- String comparison, verifier matching, selector, dump, and printer paths under
  `src/backend/bir`.
- Semantic lookup paths where BIR already has `TextId`, `LinkNameId`, block
  label IDs, structured type IDs, typed records, or prepared metadata.
- Tests that still protect drifted legacy spelling as semantic identity.

## Non-Goals

- Parser, HIR, or LIR producer migration except as separately opened blockers.
- MIR or target assembly rewrites.
- Removing dump text, diagnostics, selector input, final spelling, or display
  strings that are not semantic lookup authority.
- Weakening backend expectations, marking supported paths unsupported, or
  adding named-case shortcuts.
- Broad type-system redesign unrelated to BIR lookup authority.

## Working Model

- Pure text lookup maps become `TextId` keyed when BIR already has interned
  text.
- Semantic lookup prefers typed BIR fields, `LinkNameId`, block label IDs,
  structured type IDs, typed records, or domain semantic tables before raw
  string comparison.
- Retained raw strings must be classified as display, selector, dump text,
  diagnostics, final spelling, compatibility payload, or unresolved boundary.
- Cross-module failures should be resolved by the intended authority contract.
  If a blocker needs missing HIR or LIR metadata, record a separate open idea
  instead of expanding this runbook.

## Execution Rules

- Keep each implementation packet behavior-preserving unless the source idea
  explicitly allows updating stale tests that preserve legacy string authority.
- Do not turn display strings, selector input, dump text, diagnostics, final
  spelling, or compatibility payloads into semantic lookup authority.
- When both structured identity and drifted legacy spelling exist, structured
  identity wins for covered BIR lookup paths.
- Update tests only when they assert legacy string authority rather than
  supported semantics.
- Every code-changing packet needs fresh build or compile proof plus a focused
  BIR or LIR-to-BIR test subset chosen by the supervisor.
- Escalate to broader backend validation when a packet changes shared BIR
  verifier, printer, symbol, call, block-label, global, or type carriers.

## Step 1: Inventory BIR String Lookup Surfaces

Goal: build the working map of BIR string-keyed lookup authority and retained
spelling surfaces.

Primary target: `src/backend/bir`

Actions:
- Inspect `std::unordered_map<std::string, ...>`, string comparison, verifier
  matching, selector, dump, and printer paths under `src/backend/bir`.
- Group findings into pure text lookup, semantic lookup, display, selector,
  dump text, diagnostics, final spelling, compatibility bridge, and unresolved
  boundary categories.
- Identify the first narrow conversion target where BIR already has stable
  `TextId`, `LinkNameId`, block label ID, structured type ID, typed record, or
  other structured metadata.
- Record packet notes in `todo.md`; do not rewrite the source idea for routine
  inventory findings.

Completion check:
- `todo.md` names the first bounded conversion packet and the category of the
  major remaining BIR string surfaces.

## Step 2: Convert Covered Pure Text Lookup Maps To TextId

Goal: move BIR-owned pure text lookup maps off raw `std::string` keys where the
lookup text is already interned.

Primary target: pure text lookup surfaces found in Step 1.

Actions:
- Replace local string-keyed text lookup authority with `TextId` keys.
- Resolve text through existing `TextTable` or stored IDs instead of
  re-rendering strings for lookup.
- Keep raw strings only for display, selector input, dumps, diagnostics, final
  spelling, or compatibility payloads.
- Add focused tests that prove stale or drifted raw spelling does not beat the
  covered text-id lookup.

Completion check:
- Covered text lookup paths use `TextId` keys.
- Focused BIR tests pass and prove the drift case for at least one converted
  text lookup surface.

## Step 3: Prefer Semantic Identity In Function, Call, And Global Paths

Goal: route BIR function, call, extern, and global lookup through existing
semantic carriers before raw strings.

Primary targets: `LinkNameId`-backed paths, typed call/function/global records,
and domain tables where metadata is already present.

Actions:
- Identify lookups where `LinkNameId`, typed BIR records, function/global
  carriers, or semantic tables already exist.
- Route semantic lookup through those structured identities before legacy
  string fallbacks.
- Preserve rendered or raw names as final spelling, diagnostics, dumps,
  selectors, or compatibility data.
- Update tests that protect drifted legacy string semantic authority.

Completion check:
- Covered call, extern, global, or function-carrier paths do not use raw
  strings as primary semantic lookup authority.
- Focused BIR or LIR-to-BIR tests prove structured identity wins when legacy
  spelling drifts.

## Step 4: Extend Structured Identity Across Blocks And Types

Goal: cover BIR edge paths that use labels, structured type IDs, or typed
records but still compare raw strings for lookup.

Primary targets: block label flows, structured type references, verifier
matching, and typed BIR records.

Actions:
- Select narrow packets from the Step 1 inventory where structured metadata is
  already available.
- Convert or demote lookup authority using the same `TextId` or semantic-ID
  rule.
- Add mismatch tests for nearby cases, not only a single known failing fixture.
- Split missing HIR or LIR metadata gaps into new open ideas when required data
  is absent.

Completion check:
- Edge-path tests show `TextId` or semantic identity survives stale raw
  spelling.
- Any out-of-scope producer gap is represented as a separate idea rather than
  hidden in this plan.

## Step 5: Classify Remaining BIR String Surfaces

Goal: make retained strings visibly non-authoritative or explicitly unresolved.

Primary target: remaining BIR string-keyed or raw-name surfaces after covered
conversions.

Actions:
- Rename helpers, parameters, comments, or tests where needed to show retained
  strings are display, selector, dump text, diagnostics, final spelling,
  compatibility payload, or unresolved-boundary surfaces.
- Avoid deleting strings needed for dumps, diagnostics, final output, selector
  input, or compatibility.
- Keep public or upstream bridge surfaces classified if they cannot be safely
  converted in this BIR runbook.

Completion check:
- Remaining BIR string lookup surfaces are classified by purpose in code,
  tests, or `todo.md`.
- No covered string map remains as silent semantic authority.

## Step 6: Reprove BIR Cleanup And Decide Closure Or Split

Goal: validate the completed BIR cleanup route and decide whether the source
idea is complete or needs a follow-up split.

Actions:
- Run the supervisor-selected broader BIR proof, escalating to LIR-to-BIR and
  backend-route tests when changed carriers cross module boundaries.
- Review the diff for testcase-overfit, unsupported downgrades, and retained
  raw-string semantic authority.
- If BIR scope is complete, prepare for lifecycle close.
- If a separate HIR or LIR metadata issue remains, write or request a new idea
  under `ideas/open/` and keep this source idea focused.

Completion check:
- Regression proof is green for the selected close scope.
- The plan owner can decide whether to close this source idea or split a
  follow-up without re-inventorying the entire BIR module.
