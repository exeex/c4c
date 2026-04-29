# LIR Legacy String Lookup Removal Convergence Runbook

Status: Active
Source Idea: ideas/open/125_lir_legacy_string_lookup_removal_convergence.md

## Purpose

Converge `src/codegen/lir` away from legacy rendered-string lookup authority.
LIR should carry text IDs and semantic IDs between HIR and BIR instead of
rediscovering identity from rendered spellings.

## Goal

Classify and reduce LIR string-keyed lookup paths while preserving strings
needed for final LLVM spelling, dumps, diagnostics, compatibility payloads, or
unresolved producer/consumer boundaries.

## Core Rule

`TextId` is text identity, not semantic authority. Use `TextId` for interned
text lookup, and use `LinkNameId`, struct-name IDs, typed LIR records, or
domain semantic tables for semantic lookup when those carriers already exist.
Rendered strings may remain only after their role is explicit.

## Read First

- `ideas/open/125_lir_legacy_string_lookup_removal_convergence.md`
- `src/codegen/lir`
- LIR and backend route tests covering direct calls, extern declarations,
  globals, function carriers, pointer/global references, and type references.
- Recent HIR cleanup history when deciding whether a string surface is
  producer metadata, compatibility spelling, or unresolved upstream data.

## Current Targets

- LIR-owned `std::unordered_map<std::string, ...>` lookup maps.
- Symbol matching, rendered-name fallback, and string comparison paths under
  `src/codegen/lir`.
- Direct-call, extern, global, function-carrier, pointer/global-reference, and
  type-reference paths where LIR still treats rendered spelling as authority.
- Tests that currently protect legacy string lookup as semantic identity.

## Non-Goals

- Parser or HIR source lookup cleanup.
- BIR verifier policy changes beyond what is required to consume structured
  LIR identity.
- Broad type-reference redesign not directly tied to lookup removal.
- MIR or target assembly behavior changes.
- Removing final emitted LLVM spelling, diagnostics, dump text, or
  compatibility payloads.
- Weakening supported tests, marking covered paths unsupported, or adding
  named-case shortcuts.

## Working Model

- Pure text lookup maps become `TextId` keyed when LIR already has interned
  text.
- Semantic lookup prefers `LinkNameId`, struct-name IDs, typed LIR records,
  structured type references, or domain semantic tables before rendered-name
  fallbacks.
- Retained rendered strings must be classified as final spelling, display,
  diagnostics, dump text, compatibility payload, or unresolved boundary.
- Cross-module failures should be resolved by the intended authority contract.
  If a blocker needs upstream HIR metadata or downstream BIR contract work,
  record a separate open idea instead of expanding this runbook.

## Execution Rules

- Keep each implementation packet behavior-preserving unless the source idea
  explicitly allows updating stale tests that preserve legacy string authority.
- Do not turn final LLVM spelling, diagnostics, dump text, or compatibility
  payloads into semantic lookup authority.
- When both structured identity and drifted rendered spelling exist, structured
  identity wins for covered lookup paths.
- Update tests only when they assert legacy rendered-name authority rather than
  supported semantics.
- Every code-changing packet needs fresh build or compile proof plus a focused
  test subset chosen by the supervisor.
- Escalate to broader LIR and LIR-to-BIR validation when a packet changes
  shared function, global, extern, call, pointer, or type carriers.

## Step 1: Inventory LIR String Lookup Surfaces

Goal: build the working map of LIR string-keyed lookup authority and retained
spelling surfaces.

Primary target: `src/codegen/lir`

Actions:
- Inspect `std::unordered_map<std::string, ...>`, string comparison, rendered
  name fallback, symbol matching, and lookup paths under `src/codegen/lir`.
- Group findings into pure text lookup, semantic lookup, final spelling,
  diagnostics/display, dump text, compatibility bridge, and unresolved boundary
  categories.
- Identify the first narrow conversion target where LIR already has stable
  `TextId`, `LinkNameId`, struct-name ID, typed record, or other structured
  metadata.
- Record packet notes in `todo.md`; do not rewrite the source idea for routine
  inventory findings.

Completion check:
- `todo.md` names the first bounded conversion packet and the category of the
  major remaining string surfaces.

## Step 2: Convert Covered Pure Text Lookup Maps To TextId

Goal: move LIR-owned pure text lookup maps off raw `std::string` keys where the
lookup text is already interned.

Primary target: pure text lookup surfaces found in Step 1.

Actions:
- Replace local string-keyed text lookup authority with `TextId` keys.
- Resolve LIR text through existing text tables or stored IDs instead of
  re-rendering names for lookup.
- Keep rendered spellings only for final LLVM output, diagnostics, dumps,
  compatibility payloads, or display.
- Add focused tests that prove stale or drifted rendered spelling does not beat
  the covered text-id lookup.

Completion check:
- Covered text lookup paths use `TextId` keys.
- Focused LIR tests pass and prove the drift case for at least one converted
  lookup surface.

## Step 3: Thread Semantic Identity Through Calls, Externs, And Globals

Goal: prefer existing semantic carriers over rendered names for LIR function,
call, extern, and global lookup paths.

Primary targets: direct calls, extern declarations, globals, function carriers,
and link-name-backed paths where metadata is already present.

Actions:
- Identify lookups where `LinkNameId`, typed call records, function/global
  carriers, or domain semantic tables already exist.
- Route semantic lookup through those structured identities before rendered
  spelling fallbacks.
- Preserve rendered names as final spelling, diagnostics, dumps, or
  compatibility data.
- Update tests that protect drifted rendered-name semantic authority.

Completion check:
- Covered call, extern, global, or function-carrier paths do not use rendered
  strings as primary semantic lookup authority.
- Focused LIR or LIR-to-BIR tests prove structured identity wins when rendered
  spelling drifts.
- `collect_fn_refs` signature-text scanning is not treated as remaining Step 3
  executor scope unless a structured producer carrier already exists; classify
  it in Step 5 as compatibility or an unresolved boundary, or split a separate
  carrier idea if that producer work becomes required.

## Step 4: Extend Structured Identity Across Pointer, Global, And Type Paths

Goal: cover LIR edge paths that commonly bridge HIR-produced identity into BIR.

Primary targets: pointer/global references, struct-name flows, structured type
references, and typed LIR records.

Actions:
- Select narrow packets from the Step 1 inventory where structured metadata is
  already available.
- Convert or demote lookup authority using the same `TextId` or semantic-ID
  rule.
- Add mismatch tests for nearby cases, not only a single known failing fixture.
- Split missing HIR metadata or downstream BIR consumer gaps into new open
  ideas when the required data is absent.

Completion check:
- Edge-path tests show `TextId` or semantic identity survives stale rendered
  spelling.
- Any out-of-scope producer or consumer gap is represented as a separate idea
  rather than hidden in this plan.

## Step 5: Classify Remaining LIR String Surfaces

Goal: make retained strings visibly non-authoritative or explicitly unresolved.

Primary target: remaining LIR string-keyed or rendered-name surfaces after
covered conversions.

Actions:
- Rename helpers, parameters, comments, or tests where needed to show retained
  string paths are final spelling, display, diagnostics, dump text,
  compatibility payload, or unresolved-boundary surfaces.
- Classify `collect_fn_refs` signature-text scanning explicitly as retained
  compatibility or an unresolved producer-carrier boundary unless a structured
  signature-reference carrier already exists.
- Avoid deleting strings needed for LLVM output, dumps, diagnostics, or
  codegen-facing compatibility.
- Keep public or BIR bridge surfaces classified if they cannot be safely
  converted in this LIR runbook.

Completion check:
- Remaining LIR string lookup surfaces are classified by purpose in code,
  tests, or `todo.md`.
- No covered string map remains as silent semantic authority.

## Step 6: Reprove LIR Cleanup And Decide Closure Or Split

Goal: validate the completed LIR cleanup route and decide whether the source
idea is complete or needs a follow-up split.

Actions:
- Run the supervisor-selected broader LIR proof, escalating to LIR-to-BIR and
  backend-route tests when changed carriers cross module boundaries.
- Review the diff for testcase-overfit, unsupported downgrades, and retained
  rendered-name semantic authority.
- If LIR scope is complete, prepare for lifecycle close.
- If a separate upstream metadata or downstream BIR contract issue remains,
  write or request a new idea under `ideas/open/` and keep this source idea
  focused.

Completion check:
- Regression proof is green for the selected close scope.
- The plan owner can decide whether to close this source idea or split a
  follow-up without re-inventorying the entire LIR module.
