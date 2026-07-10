# RV64 Packed Local Member Offsets Runbook

Status: Active
Source Idea: ideas/open/667_rv64_packed_local_member_offsets.md

## Purpose

Repair RV64 lowering for packed local member offsets without merging the work
into adjacent object-data, object-emission, byval, or callee-saved routes.

## Goal

Make the focused packed-local-member runtime row pass or fail closed at the
proven first owner, using explicit packed layout and local object address
facts rather than testcase-shaped offsets.

## Core Rule

Do not hard-code offsets, field names, filenames, or final assembly shapes as
authority. A repair must be driven by semantic/prepared layout and address
facts and must keep unrelated backend baseline families separate.

## Read First

- `ideas/open/667_rv64_packed_local_member_offsets.md`
- `docs/backend_baseline_history_triage/failure_classification.md`
- `docs/backend_baseline_history_triage/follow_up_order.md`

## Current Target

- Focused baseline row: `backend_rv64_runtime_packed_local_member_offsets`
- Owning layer: RV64 packed local member offset lowering
- Proof surface: current baseline row 236 from
  `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`

## Non-Goals

- Do not work on static local object-data storage, byval payload preservation,
  pointer-local postincrement, callee-saved GPR preservation, destination
  publication, AArch64, CLI, RISC-V object emission, or LLVM torture diagnosis.
- Do not change test expectations, unsupported markers, allowlists, timeouts,
  runtime policy, or baseline accounting.
- Do not claim final assembly is sufficient proof when packed layout or local
  object address facts are missing.

## Working Model

The first owner is expected to be one of:

- packed layout publication
- local object base selection
- member offset arithmetic
- load/store width
- RV64 address materialization

The execution route must identify the first failing boundary before repairing
code.

## Execution Rules

- Keep each implementation packet narrow and tied to one proven owner.
- Preserve nearby backend families as regression surfaces only unless focused
  evidence proves they share the same first owner.
- Prefer failing closed with a precise diagnostic over guessing an address.
- Treat expectation rewrites, unsupported-marker downgrades, helper renames,
  or classification-only edits as non-progress for this idea.
- For code-changing steps, use the validation ladder chosen by the supervisor:
  build proof, focused row proof, then broader backend regression proof when
  the blast radius justifies it.

## Ordered Steps

### Step 1: Refresh Focused Evidence

Goal: Reproduce the current packed-local-member failure and capture the facts
needed to locate the first owner.

Primary target:

- `backend_rv64_runtime_packed_local_member_offsets`

Actions:

- Refresh semantic, prepared, RV64 assembly, object, and runtime evidence for
  the focused row.
- Identify the source object, packed member access, prepared layout facts,
  local object base facts, RV64 address expression, load/store width, and
  runtime mismatch.
- Record whether the first observable failure is in layout publication, object
  base selection, offset arithmetic, width selection, or RV64 address
  materialization.

Completion check:

- The executor can name one first owner with concrete evidence, or can state
  that the row already fails closed before repair with a precise diagnostic.

### Step 2: Repair The Proven Packed-Member Boundary

Goal: Implement one general packed local member offset rule at the proven
owner.

Primary targets:

- The semantic/prepared/RV64 lowering files identified by Step 1 evidence.

Actions:

- Repair only the boundary proven by Step 1.
- Use explicit packed layout and local object address facts to compute or
  materialize member offsets.
- Preserve load/store width correctness for packed members.
- Avoid testcase names, hard-coded field offsets, source-order assumptions,
  and final-assembly-only authority.

Completion check:

- The focused row either passes under the selected proof command or fails
  closed at a later, precisely named owner.
- Nearby same-feature packed-member cases, if present in the focused subset,
  do not regress.

### Step 3: Prove Backend Safety

Goal: Show the packed-member repair did not introduce broader backend
regressions.

Actions:

- Run the supervisor-selected build and focused proof command.
- Run the supervisor-selected broader backend regression subset when requested
  or when the changed files affect shared lowering.
- Keep canonical proof in `test_after.log` unless the supervisor delegates a
  different artifact.

Completion check:

- Fresh proof is recorded for the focused row and any broader subset selected
  by the supervisor.
- Any remaining failure is outside this idea's accepted scope or is recorded
  as a precise follow-up owner rather than hidden by expectations.

## Completion Criteria

- Focused evidence names the first packed local member offset owner.
- The selected repair uses explicit packed layout and local object address
  facts rather than hard-coded offsets or testcase names.
- The focused runtime row passes or fails closed with a precise diagnostic.
- Backend regression proof shows no new backend failures in the supervisor's
  chosen subset.
