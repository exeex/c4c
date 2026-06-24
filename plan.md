# RV64 Pointer-Typed Select Publication Self-Move Runbook

Status: Active
Source Idea: ideas/open/344_rv64_pointer_typed_select_publication_self_move.md
Activated from: ideas/open/343_rv64_consteval_inline_asm_template_strings.md

## Purpose

Resolve the unrelated RV64 backend-route failure that blocks full-suite proof
for the parked consteval inline asm template string runbook.

## Goal

Make `backend_codegen_route_riscv64_pointer_typed_select_publication` pass
without accepting or preserving the forbidden `mv t0, t0` self-move.

## Core Rule

Fix the backend route that produces or preserves the redundant self-move. Do
not weaken the public test, baseline logs, or forbidden-snippet contract to make
the failure disappear.

## Read First

- `ideas/open/344_rv64_pointer_typed_select_publication_self_move.md`
- `tests/backend/CMakeLists.txt`
- The source fixture for `riscv64_pointer_typed_select_publication`
- The generated failing assembly named in the latest broad proof log, if still
  present under `build/tests/backend/`

## Current Targets

- RV64 backend codegen route for pointer-typed select publication.
- Register assignment, move emission, and cleanup/peephole stages that can
  produce or remove register-to-itself moves.
- Focused backend tests around select chains, pointer typed select, and RV64
  publication output.

## Non-Goals

- Do not change consteval inline asm template string behavior.
- Do not weaken, delete, or reclassify forbidden-snippet checks.
- Do not add a test-name, filename, or exact-output shortcut for this fixture.
- Do not accept `test_baseline.new.log` as lifecycle proof.
- Do not perform a broad backend rewrite unless focused inspection proves the
  self-move path requires it.

## Working Model

- Treat `mv t0, t0` as redundant backend output that should be avoided during
  lowering or removed by a general cleanup path.
- Prefer a small semantic fix in the affected move, register, or peephole route.
- Use the failing publication test as the first proof, then cover nearby select
  or RV64 backend routes before handing broad validation back to the supervisor.

## Execution Rules

- Inspect the failing fixture and generated assembly before editing code.
- Trace where the self-move is introduced or preserved.
- Keep the forbidden-snippet expectation intact.
- Do not claim progress from baseline updates or output rewrites alone.
- Record focused proof and any remaining broad-proof requirements in `todo.md`.

## Step 1: Reproduce And Localize The Self-Move

Goal: Confirm the focused failure and identify the backend stage that emits or
preserves `mv t0, t0`.

Primary targets:

- `tests/backend/CMakeLists.txt`
- The `riscv64_pointer_typed_select_publication` fixture.
- Generated assembly under `build/tests/backend/`.
- RV64 move emission and peephole/cleanup code.

Actions:

- Run the focused backend publication test or an equivalent narrow CTest
  command selected by the supervisor.
- Inspect the generated assembly around `mv t0, t0`.
- Trace whether the self-move comes from lowering, register assignment,
  selection-chain repair, or a missing cleanup pass.
- Record the suspected owning code path in `todo.md`.

Completion check:

- The executor can name the code path that creates or fails to remove the
  self-move.
- The test contract remains unchanged.

## Step 2: Repair The General Self-Move Path

Goal: Remove or avoid redundant register-to-itself moves through the owning
backend path.

Primary targets:

- The backend route identified in Step 1.
- Existing RV64 move/peephole helpers, if they already cover this kind of
  cleanup.

Actions:

- Implement the smallest semantic fix that avoids or removes self-moves for the
  affected route.
- Prefer reusing or extending existing cleanup helpers over adding local
  string-based output filtering.
- Preserve meaningful moves between distinct registers.
- Do not special-case the failing fixture or test name.

Completion check:

- `backend_codegen_route_riscv64_pointer_typed_select_publication` no longer
  emits `mv t0, t0`.
- The diff explains a general backend behavior, not a testcase-shaped bypass.

## Step 3: Prove Nearby Backend Routes

Goal: Show the self-move fix does not regress adjacent RV64 backend behavior.

Actions:

- Run the focused failing test.
- Run nearby RV64 backend route tests for pointer typed select, select chains,
  and publication output as selected by the supervisor.
- Keep all forbidden-snippet checks intact.
- Record exact commands and results in `todo.md`.

Completion check:

- Focused and nearby backend tests pass.
- No expectation downgrade or baseline-only proof is used.

## Step 4: Hand Back For Broad Proof

Goal: Prepare the supervisor to rerun full CTest and return to the parked
consteval inline asm template string close review.

Actions:

- Summarize the fixed backend route, proof commands, and residual risk in
  `todo.md`.
- Note whether the same full CTest command that failed Step 6 should now be
  rerun by the supervisor or an executor.
- Leave the parked consteval inline asm template string source idea open until
  broad proof succeeds or is explicitly dispositioned.

Completion check:

- The backend self-move idea has focused proof.
- The supervisor has a clear next broad-proof command and can decide whether to
  close this blocker and resume close review for the parked idea.
