# BIR Raw Label Fallback Cleanup After Assembler Id Path Runbook

Status: Active
Source Idea: ideas/open/120_bir_raw_label_fallback_cleanup_after_assembler_id_path.md
Activated From: ideas/open/120_bir_raw_label_fallback_cleanup_after_assembler_id_path.md

## Purpose

Clean up raw BIR label string fallback dependencies now that the assembler and
backend path can carry structured `BlockLabelId` identity from idea 119.

## Goal

Remove or narrow raw-label fallback authority while preserving current BIR dump
spelling and keeping any retained fallback behavior explicit and tested.

## Core Rule

Do not remove a raw string fallback until the affected assembler/backend path is
proven to consume `BlockLabelId` or has an explicit compatibility boundary.

## Read First

- `ideas/open/120_bir_raw_label_fallback_cleanup_after_assembler_id_path.md`
- `ideas/closed/119_bir_block_label_structured_identity_for_assembler.md`
- `src/shared/text_id_table.hpp`
- BIR IR, printer, preparation, and backend handoff files touched by the
  structured label id path.

## Current Targets

- BIR block labels, branch targets, conditional branch targets, phi incoming
  labels, and label-address bases that still store or trust raw strings.
- BIR construction and validation paths that can still create raw-only BIR.
- CLI focus filters and matching for function, block, and value selectors.
- Prepared liveness, stack layout, dynamic stack, out-of-SSA, and assembler
  consumption paths that still intern or match labels from raw spelling.
- Tests that prove valid id rendering and intentionally retained fallback
  behavior.

## Non-Goals

- Do not change BIR dump spelling unless the supervisor explicitly accepts a
  contract change.
- Do not migrate MIR target codegen internals as part of this runbook if it
  grows beyond raw fallback cleanup.
- Do not weaken or mark supported-path tests unsupported to make cleanup pass.
- Do not fabricate ids for unresolved raw-only paths.

## Working Model

- `BlockLabelId` is the preferred authority for backend label identity.
- Raw label strings may remain only as rendering compatibility or as explicit
  boundaries where an upstream consumer has not been converted yet.
- Same-spelled labels across functions must remain compatible with the module
  table model established by idea 119.
- Unresolved ids are proof gaps and must be surfaced in code, tests, or
  `todo.md`, not hidden behind broader fallback matching.

## Execution Rules

- Prefer structured id lookup over string matching whenever a table is already
  available.
- Keep changes behavior-preserving for accepted paths.
- When a fallback remains, make the boundary narrow, named, and covered by a
  focused test.
- For every code-changing step, run a fresh build or compile proof plus the
  narrow backend tests relevant to the touched surface.
- Escalate to a broader backend test run before treating the runbook as
  complete, because label identity crosses BIR printing, preparation, and
  assembler handoff.

## Ordered Steps

### Step 1: Inventory Raw Label Fallbacks And Proof Baseline

Goal: map current raw-label dependencies before removing any fallback.

Primary targets:
- BIR IR fields and constructors for blocks, branch terminators, conditional
  branch terminators, phi incoming labels, and label-address operands.
- BIR and prepared printers.
- CLI focus filter routing.
- Prepared liveness, stack-layout, dynamic-stack, out-of-SSA, and assembler
  handoff paths.
- Existing backend tests that mutate or assert raw label spelling.

Concrete actions:
- Inspect the current structured id path from BIR construction through prepared
  backend consumption.
- Classify each raw-label use as rendering compatibility, construction
  convenience, unresolved-id fallback, or legacy downstream authority.
- Identify a narrow proof command for the first cleanup packet and record it in
  `todo.md`.
- Do not edit implementation files in this step unless the supervisor delegates
  execution after activation.

Completion check:
- `todo.md` identifies the first executable cleanup packet, the target files,
  the expected fallback classification, and the narrow proof command.

### Step 2: Narrow BIR Construction And Validation Fallbacks

Goal: prevent new raw-only BIR from bypassing structured label identity except
at explicit compatibility boundaries.

Primary targets:
- BIR construction helpers, lowering or interning boundary code, and validation
  tests that still create raw-only blocks or edges.

Concrete actions:
- Convert construction helpers and fixtures to attach `BlockLabelId` where the
  table is available.
- Keep raw spelling as display text or compatibility payload only when the id is
  invalid by design.
- Add or update tests that prove id-backed labels render the same text and that
  retained raw-only construction behavior is intentional.

Completion check:
- New or existing BIR construction paths attach ids for ordinary blocks and
  edges, and raw-only construction is either gone or covered as explicit
  compatibility behavior.

### Step 3: Prefer Id Authority In Printing And Focus Matching

Goal: make rendering and focused dump selection use structured identity where
available without changing user-visible dump text.

Primary targets:
- BIR printer and prepared printer label rendering.
- CLI focus filters for function, block, and value selectors in BIR, prepared
  BIR, MIR, or trace routes.

Concrete actions:
- Prefer `BlockLabelId` table lookup for label rendering and block matching.
- Retain raw spelling fallback only for invalid or intentionally raw-only test
  inputs.
- Preserve focused dump behavior and spelling for branch, conditional branch,
  phi incoming, focus-block, and focus-value cases.

Completion check:
- Focused dump tests still pass, and any raw label comparison that remains is
  either a selector spelling boundary or an invalid-id compatibility fallback.

### Step 4: Convert Prepared Pipeline Consumers To Id-First Lookup

Goal: remove raw string authority from prepared liveness, stack layout,
dynamic-stack, out-of-SSA, and related backend preparation paths where ids are
now sufficient.

Primary targets:
- `src/backend/prealloc/liveness.cpp`
- `src/backend/prealloc/stack_layout/coordinator.cpp`
- `src/backend/prealloc/prealloc.cpp`
- `src/backend/prealloc/out_of_ssa.cpp`
- Related prepared data model and tests.

Concrete actions:
- Replace repeated raw-label interning or string matching with existing
  `BlockLabelId` fields and table lookups.
- Keep raw strings only for dump text or for a documented legacy boundary.
- Add focused tests for same-spelled labels across functions where the
  conversion could otherwise collapse identity.

Completion check:
- Prepared pipeline consumers use ids as authority for ordinary label control
  flow, and narrow backend tests prove branch, conditional branch, phi incoming,
  and label-address behavior.

### Step 5: Prove Assembler Handoff And Document Retained Boundaries

Goal: verify assembler/backend consumption no longer depends on raw label
spelling for the cleaned paths.

Primary targets:
- Backend handoff routes from prepared BIR into assembler or target-local code.
- Tests that exercise codegen asm with prepared BIR.

Concrete actions:
- Trace label identity through the assembler handoff for cleaned paths.
- Add assertions or tests that fail when a required id is missing.
- Document retained raw-label boundaries in `todo.md` with the upstream
  consumer that still requires them.

Completion check:
- Assembler/backend handoff proof demonstrates id consumption for affected
  paths, and all retained raw fallback behavior has an explicit reason.

### Step 6: Final Validation And Completion Review

Goal: prove the cleanup is stable across the backend label pipeline and prepare
the source idea for closure judgment.

Primary targets:
- Backend build and backend test suite.
- Any focused BIR or prepared BIR dump tests touched by prior steps.

Concrete actions:
- Run the accepted narrow proof commands from prior steps.
- Run a broader backend validation command selected by the supervisor.
- Review remaining raw-label fallback sites against the source idea acceptance
  criteria.
- Record any intentionally retained boundaries and proof results in `todo.md`.

Completion check:
- Fresh validation is recorded, no testcase-overfit route remains, and the
  source idea acceptance criteria can be evaluated by the plan owner.
