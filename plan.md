# RISC-V Memory Accesses Stale-Publication Fail-Closed Proof

Status: Active
Source Idea: ideas/open/276_phase_f5_riscv_memory_accesses_stale_publication_fail_closed_proof.md

## Purpose

Use the supported stale-publication fixture from idea 275 to prove that one
bounded RISC-V same-consumer public `PreparedFunctionLookups::memory_accesses`
row fails closed when it is stale relative to the current Route 3 / Route 5
memory-source authority.

## Goal

Exercise the real RISC-V same-consumer backend path, reject the stale public
memory row as semantic agreement, and preserve the compatible `LoadLocal`
positive path with exact `lw a1, 12(s2)` output.

## Core Rule

Do not accept helper-only classification, synthetic post-construction mutation,
expectation weakening, or named-case shortcuts as proof. The fail-closed
evidence must come from the supported fixture and the real backend consumer
path.

## Read First

- `ideas/open/276_phase_f5_riscv_memory_accesses_stale_publication_fail_closed_proof.md`
- `ideas/closed/275_riscv_memory_accesses_stale_publication_fixture_support.md`
- The supported stale public `memory_accesses` fixture added by idea 275.
- Existing RISC-V same-consumer `LoadLocal` tests and exact-output checks.
- Route 3 / Route 5 memory-source agreement code that consumes
  `PreparedFunctionLookups::memory_accesses`.
- Prepared-memory status, debug, route-debug, printer, wrapper, fallback, and
  baseline assertions for the fixture family.

## Current Targets And Scope

- Reuse the supported stale public `PreparedFunctionLookups::memory_accesses`
  fixture from idea 275.
- Prove one RISC-V same-consumer stale public row is rejected on the real
  backend path.
- Preserve the compatible `LoadLocal` path and exact `lw a1, 12(s2)` output.
- Record evidence that names the stale public row and the current Route 3 /
  Route 5 authority relation.
- Keep the proof bounded to the stale-publication family.

## Non-Goals

- Do not broaden this into cross-publication, exhaustive byte-offset coverage,
  x86 parity, edge-publication families, metadata, liveness, aggregate
  retirement, draft 155 work, or broad Route 3 / Route 5 rewrites.
- Do not introduce synthetic post-construction mutation as proof evidence.
- Do not demote, privatize, wrap, delete, or retire prepared aggregate APIs.
- Do not weaken fallback, helper/oracle status, route-debug,
  prepared-printer, wrapper, exact target output, unsupported, or baseline
  behavior.

## Working Model

Start from the supported RISC-V stale-publication fixture built for the
dynamic stack-source `LoadLocal` path. Confirm the fixture exposes both the
stale public prepared memory row and the current Route 3 / Route 5 authority
row. Then add the narrow backend proof that the stale row is rejected while the
compatible positive row still renders exactly `lw a1, 12(s2)`.

## Execution Rules

- Prefer semantic backend proof over testcase-shaped matching.
- Keep helper/oracle evidence supplemental unless the real RISC-V backend path
  is also exercised.
- Preserve positive output byte-for-byte.
- Make the stale public row and current Route 3 / Route 5 authority row
  distinguishable in status/debug evidence.
- Any code-changing step needs fresh build or compile proof plus the
  supervisor-delegated narrow test command.
- Escalate to reviewer or lifecycle repair if the route requires broad Route 3
  / Route 5 rewrites, expectation downgrades, fixture mutation, or prepared API
  retirement.

## Step 1: Locate The Supported Stale Fixture

Goal: Confirm the supported fixture from idea 275 is available and identify
the real backend consumer path it should drive.

Primary target: Existing RISC-V same-consumer `LoadLocal` fixture and
prepared `memory_accesses` publication tests.

Actions:

- Locate the supported stale public `PreparedFunctionLookups::memory_accesses`
  fixture and the compatible `LoadLocal` positive fixture.
- Identify the stale public row, the current Route 3 / Route 5 authority row,
  and the assertions or debug facts that distinguish them.
- Trace the real RISC-V backend path that consumes the public prepared memory
  row for the same consumer.
- Record the named fixtures, rows, consumer path, and proof target in
  `todo.md`.

Completion check:

- `todo.md` names the supported stale fixture, compatible positive fixture,
  stale public row, current Route 3 / Route 5 authority row, and backend
  consumer path.
- No implementation files are changed in this inspection step unless the
  supervisor explicitly delegates Step 2 in the same packet.

## Step 2: Add The Stale Rejection Proof

Goal: Prove the stale public `memory_accesses` row fails closed on the real
RISC-V same-consumer backend path.

Primary target: Narrow RISC-V memory-access backend proof for the supported
stale-publication fixture.

Actions:

- Add or extend the narrow test that drives the supported stale fixture through
  the real backend consumer.
- Assert that the stale public row is rejected instead of rendering an
  accepted instruction for that stale row.
- Assert status/debug evidence names the stale public row and the current
  Route 3 / Route 5 memory-source authority relation.
- Keep helper/oracle checks aligned with backend behavior rather than using
  them as the sole proof.

Completion check:

- The stale public `memory_accesses` row fails closed through the real backend
  path.
- Evidence distinguishes stale-publication rejection from missing fixture
  support.
- No fallback, status/debug, helper/oracle, route-debug, prepared-printer,
  wrapper, exact-output, or baseline contract is weakened.

## Step 3: Preserve The Compatible Positive Path

Goal: Prove the compatible RISC-V same-consumer path remains byte-stable while
the stale row fails closed.

Primary target: Compatible dynamic stack-source `LoadLocal` exact-output
coverage for the same fixture family.

Actions:

- Assert the compatible positive path still emits exactly `lw a1, 12(s2)`.
- Keep positive fixture construction on the supported path.
- Verify stale rejection does not relax exact-output, wrapper, fallback,
  prepared-printer, route-debug, helper/oracle, unsupported, or baseline
  expectations.

Completion check:

- The positive output remains exactly `lw a1, 12(s2)`.
- The stale rejection proof and compatible positive proof can run together
  without expectation downgrades.

## Step 4: Run Acceptance Proof

Goal: Provide fresh proof that the stale-publication fail-closed behavior works
and the compatible same-consumer path did not regress.

Primary target: Supervisor-selected build plus narrow RISC-V memory-access
test subset.

Actions:

- Run the delegated build or compile proof.
- Run the delegated narrow test command for this fixture family.
- If shared Route 3 / Route 5 behavior changed, ask the supervisor to select
  broader validation before accepting the slice.
- Record commands and results in `todo.md`.

Completion check:

- Build or compile proof is fresh.
- Narrow stale-publication and compatible-path proofs are green.
- No status, debug, fallback, prepared-printer, wrapper, exact-output,
  unsupported, helper/oracle, route-debug, or baseline contract was weakened.
