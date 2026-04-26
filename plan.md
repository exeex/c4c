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
- Bounded assembler/backend handoff proof surfaces that can exercise prepared
  control-flow label ids without rebuilding target scalar rendering.
- Tests that prove valid id rendering and intentionally retained fallback
  behavior.

## Non-Goals

- Do not change BIR dump spelling unless the supervisor explicitly accepts a
  contract change.
- Do not migrate MIR target codegen internals as part of this runbook if it
  grows beyond raw fallback cleanup.
- Do not recover broad x86 scalar/module rendering as the means of proving
  label-id handoff; that is tracked separately in
  `ideas/open/121_x86_prepared_module_renderer_recovery.md`.
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
- Treat `review/step5_x86_handoff_dirty_slice_review.md` as the formal route
  reset basis for Step 5. The rejected x86 renderer recovery attempt is not
  accepted progress for this plan.

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

### Step 5: Prove Bounded Assembler Handoff Boundaries

Goal: verify assembler/backend consumers of the cleaned prepared-control-flow
paths use structured label authority without expanding into target renderer
recovery.

Primary targets:
- Prepared-control-flow handoff routes that already carry `BlockLabelId` or
  prepared block identity from Step 4.
- Existing assembler/backend boundary tests or a restored x86 handoff test
  surface, limited to id consumption and missing/drifted-id rejection.

Concrete actions:
- Start from the accepted Step 4 prepared-consumer state, not from the rejected
  x86 renderer dirty slice.
- Use `review/step5_x86_handoff_dirty_slice_review.md` as the route-reset
  record when explaining why broad x86 renderer recovery is out of scope.
- Trace label identity through a bounded handoff path that can observe prepared
  branch/conditional-branch labels and reject missing or drifted ids.
- If the optional x86 handoff test infrastructure is restored, keep the packet
  limited to compile compatibility and label-id handoff assertions. Do not add
  handwritten scalar BIR-shape dispatch in `module.cpp` or recover unrelated
  x86 scalar emission to reach the assertion.
- If no bounded assembler/backend proof surface exists in this checkout, stop
  with a Step 5 blocker and ask the supervisor to choose between restoring
  handoff infrastructure and switching to the separate x86 renderer initiative.
- Document retained raw-label boundaries in `todo.md` with the upstream
  consumer that still requires them.

Completion check:
- A bounded handoff proof demonstrates id consumption and missing/drifted-id
  rejection for affected prepared-control-flow paths, or `todo.md` records the
  exact missing proof surface as a blocker. No broad target renderer recovery
  or testcase-shaped scalar emission is counted as Step 5 progress.

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
