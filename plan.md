# RV64 Branch Residual Terminator Fragment Lowering Runbook

Status: Active
Source Idea: ideas/open/645_rv64_branch_residual_terminator_fragment_lowering.md

## Purpose

Repair or precisely classify the remaining RV64 object-route branch residuals
whose first owner is now terminator fragment lowering.

## Goal

Advance at least one supported same-family branch terminator shape through
semantic RV64 lowering, or reclassify all representative rows to more precise
owners with current evidence.

## Core Rule

Do not accept branch stack operands by apparent stack offsets, frame homes,
final assembly shape, or assumed no-clobber behavior. Terminator lowering may
consume branch operands only when required freshness and clobber-safety
authority is explicit.

## Read First

- ideas/open/645_rv64_branch_residual_terminator_fragment_lowering.md
- ideas/closed/611_rv64_terminator_fragment_lowering.md
- ideas/closed/635_prepared_branch_stack_clobber_safety_authority.md
- ideas/closed/636_prepared_branch_stack_source_freshness_publication.md
- docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md
- docs/rv64_gcc_torture_1000_pass_recovery/dependency_order_to_1000.md

## Current Scope

- Refresh diagnostics for the eight branch residual rows named by the source
  idea.
- Group each row by concrete BIR terminator shape and first unsupported
  lowering boundary.
- Compare the grouped shapes with existing RV64 terminator-fragment support.
- Add semantic RV64 object-route terminator lowering only for a shape with
  explicit branch-source freshness and clobber-safety evidence.

## Non-Goals

- Do not publish branch stack-source freshness or clobber-safety authority.
- Do not reopen parameter ABI/home admission for `src/20001017-1.c`.
- Do not change select publication, compare publication, runtime policy,
  expectations, unsupported markers, allowlists, timeouts, or accounting.
- Do not add named-case handling for any representative source file.

## Working Model

The representative rows now stop at `unsupported_terminator_fragment` after
clobber-safety or freshness authority is no longer the first owner. The next
work is to determine whether the unsupported boundary is a shared semantic
terminator shape RV64 can lower, or whether each row belongs to a more precise
owner outside this idea.

## Execution Rules

- Keep routine execution notes in `todo.md`.
- Start with probes and diagnostics before changing RV64 lowering.
- Prefer shape-based coverage over source-file-specific assertions.
- Preserve fail-closed behavior for missing freshness, missing
  clobber-safety, unsupported branch operands, and unrelated terminator forms.
- Any code-changing acceptance slice needs fresh build proof and the
  supervisor-selected regression comparison.

## Steps

### Step 1: Refresh Residual Terminator Evidence

Goal: identify the current first unsupported boundary and concrete BIR
terminator shape for each representative row.

Actions:
- Refresh object-route diagnostics for `src/loop-2e.c`, `src/pr39100.c`,
  `src/20000314-3.c`, `src/20140828-1.c`, `src/20080519-1.c`,
  `src/20050125-1.c`, `src/930930-1.c`, and `src/20060910-1.c`.
- Capture the BIR terminator shape, branch operand source, and required
  freshness or clobber-safety evidence for each row.
- Group rows by semantic terminator shape and first unsupported lowering
  boundary.

Completion check:
- `todo.md` records the grouped residual shapes and identifies whether at
  least one shared terminator family is in scope for semantic RV64 lowering.

### Step 2: Compare Against Existing Terminator Support

Goal: decide whether the grouped shape is a missing RV64 lowering case or a
  more precise non-terminator owner.

Actions:
- Compare the grouped shapes with existing RV64 terminator-fragment support and
  closed idea 611 evidence.
- Identify the smallest semantic lowering target that can be implemented
  without weakening freshness or clobber-safety contracts.
- If no target is in scope, record the precise owner for each row.

Completion check:
- `todo.md` names the selected in-scope semantic shape and proof surface, or
  records a precise reclassification for all rows.

### Step 3: Lower One Supported Same-Family Shape

Goal: add RV64 object-route lowering for one semantic terminator shape using
only explicit prepared authority.

Actions:
- Implement the narrow lowering path for the selected shape.
- Require explicit branch-source freshness and clobber-safety evidence where
  the operand family needs it.
- Keep unsupported operands and unrelated terminator forms rejected with
  precise diagnostics.
- Add focused positive and negative coverage for the shape, not a named row.

Completion check:
- The narrow build/test proof passes, and at least one representative row
  advances past `unsupported_terminator_fragment` without expectation or
  allowlist churn.

### Step 4: Validate Boundaries And Residual Owners

Goal: prove the route is not a testcase-shaped shortcut and does not weaken
branch authority contracts.

Actions:
- Re-run the supervisor-selected RV64 object and branch-authority subsets.
- Compare before/after logs with the regression guard.
- Inspect nearby same-family branch rows affected by the shared lowering.
- Record any remaining rows and their precise owners in `todo.md`.

Completion check:
- Regression proof is non-regressing, negative freshness and clobber-safety
  cases remain rejected, and remaining residuals are assigned to explicit
  owners.

### Step 5: Final Lifecycle Review

Goal: decide whether the terminator-fragment idea is complete.

Actions:
- Compare final behavior against the source idea acceptance criteria and
  reviewer reject signals.
- Close only if at least one same-family terminator shape advanced through
  semantic RV64 lowering, or all representative rows were reclassified to more
  precise owners with current evidence.

Completion check:
- Plan owner can close, deactivate, or split the lifecycle state with matching
  validation evidence.
