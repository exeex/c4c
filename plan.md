# Control-Flow Branch-Target Identity Runbook

Status: Active
Source Idea: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md

## Purpose

Activate exactly one remaining idea 260 candidate: the `control_flow`
branch-target identity packet.

## Goal

Let prepared branch-target helper code use BIR conditional true/false
`BlockLabelId` identity only when it agrees with the prepared control-flow
surface, while preserving current public prepared fallback behavior.

## Core Rule

Do not migrate unrelated control-flow readers or weaken fallback/output
contracts. Every BIR-derived branch-target fact must pass through a local
agreement boundary and fail closed to the current prepared behavior.

## Read First

- `ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/calls.hpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

## Current Target

Selected candidate:

- `control_flow` branch-target identity packet:
  prepared branch-target helpers may use BIR conditional true/false
  `BlockLabelId` values only under prepared agreement, while preserving public
  fallback to prepared labels and fail-closed behavior for non-conditional,
  absent, invalid, missing structured-id, mismatch, and non-agreement rows.

## Non-Goals

- Do not reactivate completed `module`, `names`, or `control_flow`
  call-preservation packets.
- Do not delete, privatize, wrap, or broadly retire any `PreparedBirModule`
  field.
- Do not rewrite printer/debug output, route-debug output, fallback strings,
  helper/oracle status names, diagnostics, baselines, or target output to
  claim progress.
- Do not change AArch64 branch lowering, fused compare, dispatch, MIR successor
  metadata, traversal/layout, x86/RISC-V setup, join/edge transfer,
  publication, parallel-copy, cycle-temp, execution-site, move-bundle readers,
  construction/mutation, public tests, or printer/debug rows outside this
  helper-row candidate.

## Working Model

- Prepared control-flow labels and public prepared aggregate compatibility
  remain authoritative.
- BIR conditional true/false labels can be treated as structural identity only
  after a prepared/BIR agreement check proves that the prepared branch-target
  row and structured BIR labels describe the same targets.
- Unsupported, absent, invalid, stale, duplicate, or mismatching inputs must
  fall through to the existing prepared-label fallback or invalid-label
  behavior.

## Execution Rules

- Keep implementation edits local to the selected helper row unless the
  supervisor explicitly delegates a narrower owned-file set.
- Prefer a private helper in `prepared_lookups.cpp` unless a public declaration
  is necessary for the selected candidate.
- Add focused helper tests before broadening behavior.
- Treat expectation rewrites, status-name changes, or output-string changes as
  route drift unless the supervisor explicitly approves them.
- Use the supervisor-provided proof command exactly for each executor packet.

## Ordered Steps

### Step 1: Locate the Branch-Target Helper Surface

Goal: identify the exact prepared branch-target helper(s), existing prepared
fallback behavior, and the available BIR conditional label facts.

Primary target:

- `src/backend/prealloc/prepared_lookups.cpp`

Actions:

- Inspect the current branch-target lookup helper and its call sites.
- Identify the existing fallback outcomes for non-conditional, absent,
  invalid, missing structured-id, mismatch, and non-agreement rows.
- Identify any nearby helper-test rows that already exercise branch targets.

Completion check:

- `todo.md` records the selected helper row, the retained prepared fallback,
  and the minimal implementation/test files needed for Step 2.

### Step 2: Add the Agreement Boundary

Goal: introduce or reuse a local prepared/BIR agreement boundary for
conditional branch-target identity.

Primary target:

- `src/backend/prealloc/prepared_lookups.cpp`

Actions:

- Compare BIR conditional true/false `BlockLabelId` values against the
  prepared branch-target row before returning a BIR-derived identity.
- Preserve public fallback to prepared labels whenever the agreement is absent
  or incomplete.
- Keep the helper behavior fail-closed for non-conditional, absent, invalid,
  missing structured-id, mismatch, and non-agreement rows.

Completion check:

- Focused helper behavior uses BIR conditional branch-target identity only when
  prepared agreement is complete, with no unrelated control-flow reader
  migration.

### Step 3: Prove Positive and Fail-Closed Rows

Goal: add focused proof for the selected candidate and nearby same-feature
fail-closed rows.

Primary target:

- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

Actions:

- Cover the positive prepared/BIR agreement path.
- Cover fail-closed rows for non-conditional, absent control-flow,
  invalid labels, missing structured-id, prepared/BIR mismatch, and
  non-agreement.
- Assert retained public prepared fallback behavior where applicable.

Completion check:

- The focused helper test passes under the supervisor-delegated proof command,
  and the test changes do not weaken existing contracts.

### Step 4: Broader Backend Validation and Retirement Check

Goal: confirm the completed one-candidate runbook is ready for supervisor
review and plan-owner retirement.

Actions:

- Run the supervisor-delegated broader backend proof after focused proof is
  green.
- Update `todo.md` with proof results and any remaining risks.
- Do not close the source idea; remaining idea 260 candidates stay open.

Completion check:

- Backend proof is green, `todo.md` marks the branch-target identity packet
  complete, and the supervisor can request plan-owner retirement of this
  active runbook.
