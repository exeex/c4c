# Type Identity Migration Closure Gate Runbook

Status: Active
Source Idea: ideas/open/182_type_identity_migration_closure_gate.md

## Purpose

Close the type-identity migration wave as a coherent milestone after the
post-audit implementation ideas have landed.

## Goal

Produce an evidence-backed closure ledger that classifies which type domains
now use structured identity, which rendered spellings remain output or
compatibility text, and whether any remaining high-risk type authority needs a
new narrow follow-up idea.

## Core Rule

This is a closure and classification gate. Do not start unrelated
implementation work, weaken tests, or treat rendered spelling as semantic
authority without explicitly classifying the boundary.

## Read First

- `ideas/open/182_type_identity_migration_closure_gate.md`
- Closed source ideas `ideas/closed/172_*` through `ideas/closed/181_*`
- Current `test_baseline.log`, if available, for accepted broad-suite state
- Current `test_before.log` / `test_after.log`, if supervisor provides a
  matching closure proof pair

## Current Scope

- Review closed ideas 172-181 and summarize the migration result by type
  domain.
- Build a type identity closure ledger covering syntax payload, resolved type
  identity, layout identity, ABI facts, display spelling, and compatibility
  bridges.
- Confirm structured facts own the selected aggregate layout, aggregate ABI,
  HIR type-ref, LIR type-ref, template record owner, global aggregate, byval
  copy, AArch64/direct-LIR, and function-pointer signature paths.
- Run or accept broad validation according to supervisor baseline policy,
  including full suite unless an explicit accepted baseline says otherwise.
- Convert any remaining blocker into an explicit follow-up idea instead of
  burying it in the closure gate.

## Dependency State

The source idea's `Depends On` list still names the original open paths for
ideas 178-181. Activation treats those dependencies as satisfied because the
corresponding files are now present under `ideas/closed/`.

## Non-Goals

- Do not start the next migration theme.
- Do not rewrite the full type system or remove all `TypeSpec` syntax payloads.
- Do not change output or diagnostic type spelling just to reduce strings.
- Do not accept weak tests, unsupported downgrades, or unexplained failures as
  closure.
- Do not edit implementation files as part of this gate.

## Working Model

- Parser and `TypeSpec` spelling can remain syntax payload.
- Resolved type identity must be owned by structured semantic facts where the
  closed ideas claim migration progress.
- Layout and ABI identity should be classified separately from display spelling
  and final emitted syntax.
- Retained string bridges are acceptable only when documented as output,
  diagnostics, ABI spelling, or explicit no-metadata compatibility boundaries.
- Any unclassified spelling-based authority is a blocker unless converted into
  a new narrow source idea.

## Execution Rules

- Prefer ledger evidence from closed ideas, current code, and validation logs
  over new implementation.
- If a gap is only documentation/classification, record it in `todo.md` and the
  final closure summary.
- If a gap requires code changes, create a new `ideas/open/` follow-up through
  supervisor/plan-owner lifecycle instead of expanding this runbook.
- For validation, follow supervisor policy and keep canonical log filenames:
  `test_before.log` and `test_after.log`; do not invent alternate root logs.
- Broad validation is required for closure unless an accepted full-suite
  baseline is explicitly cited.

## Ordered Steps

### Step 1: Verify Closure Inputs

Goal: confirm the gate has all required source and validation inputs.

Actions:

- Check that closed ideas 172-181 are available and summarize their closure
  claims.
- Record the available broad validation or baseline artifacts.
- Identify whether a fresh broad validation command is required by supervisor
  policy.
- Confirm no review or baseline reminder is pending in `todo.md`.

Completion Check:

- `todo.md` records the dependency/validation input state and the exact
  validation path expected for closure.

### Step 2: Build the Type-Domain Closure Ledger

Goal: document the migration state for each type-identity domain.

Actions:

- For each domain from closed ideas 172-181, classify syntax payload, resolved
  type identity, layout identity, ABI facts, display spelling, and
  compatibility behavior.
- Identify where structured facts are authoritative.
- Identify every retained rendered string surface and classify it as output,
  diagnostic text, ABI spelling, or no-metadata compatibility bridge.
- Mark any unresolved or ambiguous authority as a blocker.

Completion Check:

- The ledger is concrete enough for a reviewer to determine whether rendered
  type spelling still has unclassified semantic authority.

### Step 3: Validate Broad Closure State

Goal: obtain or accept closure-quality validation for the type-identity wave.

Actions:

- Run the broad validation command chosen by the supervisor, or cite the
  accepted full-suite baseline if supervisor policy allows it.
- If using regression guard logs, ensure before/after scopes match exactly.
- Explain any baseline failures, skipped tests, or policy exceptions.
- Reject closure if validation is missing or unexplained.

Completion Check:

- `todo.md` contains the accepted validation evidence and baseline policy.

### Step 4: Convert Remaining Blockers

Goal: ensure closure does not hide unfinished type-identity work.

Actions:

- Review ledger blockers and decide whether each is already out of scope,
  explicitly compatible, or requires follow-up.
- For true remaining blockers, create or request creation of narrow
  `ideas/open/` follow-up ideas with concrete reviewer reject signals.
- Do not perform implementation work inside this closure gate.

Completion Check:

- No high-risk spelling-based type authority remains unclassified; any true
  blocker has a follow-up idea or prevents closure.

### Step 5: Summarize Closure Decision

Goal: prepare the lifecycle decision for plan-owner closure.

Actions:

- Summarize the accepted ledger, validation result, retained compatibility
  bridges, and follow-up ideas if any.
- State whether the source idea acceptance criteria are satisfied.
- If satisfied, route to plan-owner closure. If not, leave this active plan in
  a coherent state explaining the remaining work.

Completion Check:

- `todo.md` contains enough evidence for a close/deactivate/split decision
  without re-deriving the whole migration history.
