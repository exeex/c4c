# RV64 Move-Bundle Target Materialization Runbook

Status: Active
Source Idea: ideas/open/610_rv64_move_bundle_target_materialization.md

## Purpose

Activate idea 610 as an executable route for RV64/MIR consumer work on
prepared out-of-SSA move-bundle target shapes.

## Goal

Implement RV64 materialization for supported move-bundle target shapes when
prepared authority already proves the move operands and destinations.

## Core Rule

Consume prepared move-bundle authority; do not infer missing source,
destination, stack-slot, branch-operand, or fan-in authority from an encodable
RV64 target shape.

## Read First

- `ideas/open/610_rv64_move_bundle_target_materialization.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/prealloc/value_locations.hpp`
- `src/backend/prealloc/control_flow.hpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`

## Current Targets

- Primary diagnostic family: `unsupported_move_bundle_target_shape`.
- Primary proof surface: prepared move-bundle rows with existing authority,
  especially `out_of_ssa_parallel_copy`.
- Representative case from the source idea: `src/20020206-2.c`.
- Main RV64 consumer surface:
  `fragment_for_prepared_move_bundle`,
  `fragment_for_prepared_out_of_ssa_moves`, and related diagnostics in
  `src/backend/mir/riscv/codegen/object_emission.cpp`.

## Non-Goals

- Do not change prepared/prealloc authority production.
- Do not implement destination fan-in policy.
- Do not weaken expectations, unsupported markers, allowlists, timeout files,
  runtime accounting, or failure buckets.
- Do not fold terminator lowering, generic instruction fragments, ABI
  calls/returns, global data, or runtime mismatch work into this plan.
- Do not add named-case-only handling for `src/20020206-2.c` or any other
  single testcase.

## Working Model

- Supported rows already have enough prepared authority for RV64 to consume.
- Unsupported rows must remain fail-closed with diagnostics that identify the
  missing or rejected authority.
- Destination fan-in rows remain blocked until their producer-side policy is
  explicitly available; RV64 must not choose ordering or exclusivity locally.

## Execution Rules

- Start each code step with diagnostics that classify the first failing
  ownership boundary before editing.
- Prefer small semantic consumer rules over broad object-emission rewrites.
- Keep diagnostics precise when a row remains unsupported.
- For code-changing steps, run at least:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
- Escalate to broader validation before close if multiple target-shape families
  are changed or if backend proof begins relying on expectation movement.

## Ordered Steps

### Step 1: Refresh Move-Bundle Residual Diagnostics

Goal: identify which current `unsupported_move_bundle_target_shape` rows are
real RV64 consumer gaps with prepared authority already present.

Actions:
- Run focused object-route probes for representative move-bundle rows,
  including `src/20020206-2.c` when available in the current harness.
- Record the first owner for each residual as RV64 consumer,
  prepared/prealloc producer, destination fan-in, or unrelated route.
- Inspect prepared move-bundle diagnostics for phase, authority kind, move
  count, source and destination home kinds, parallel-copy metadata, and
  failure status.

Completion check:
- `todo.md` names the narrow first implementation packet and the rows it is
  allowed to move.
- Rows lacking prepared source/destination authority or unresolved fan-in are
  explicitly excluded from the code packet.

### Step 2: Add The First Supported RV64 Materialization Rule

Goal: consume one real supported move-bundle target shape without bypassing
prepared authority.

Primary target:
- `src/backend/mir/riscv/codegen/object_emission.cpp`

Actions:
- Extend the RV64 prepared move-bundle consumer for the smallest authorized
  out-of-SSA target shape found in Step 1.
- Reuse existing register, stack-slot, immediate, parallel-copy, and storage
  coherence helpers where possible.
- Preserve fail-closed behavior for missing homes, ambiguous authority,
  destination fan-in, unsupported widths, cycle temporary gaps, and non-move
  operations.
- Add or update focused backend tests only when they prove the semantic
  consumer rule rather than rewriting expectations for a narrow testcase.

Completion check:
- At least one same-family authorized move-bundle row progresses past the prior
  RV64 target-shape failure.
- Negative rows still fail with owner-specific diagnostics.
- Backend subset proof passes.

### Step 3: Broaden Same-Family Coverage

Goal: generalize the first rule across nearby authorized move-bundle shapes
without crossing into other ideas.

Actions:
- Expand only across shapes that share the same prepared authority contract.
- Cover register-to-register, stack-to-register, register-to-stack,
  stack-to-stack, immediate, or FPR cases only when Step 1 evidence shows they
  belong to idea 610 and prepared authority is complete.
- Keep destination fan-in, select publication source wiring, terminator
  fragments, ABI return moves, and generic instruction fragments out of scope.

Completion check:
- Multiple move-bundle target-shape rows progress through RV64 lowering.
- Any remaining rows are classified by missing authority, blocked fan-in, or a
  separate open idea rather than hidden behind a generic target-shape message.

### Step 4: Tighten Diagnostics And Regression Boundaries

Goal: make the remaining unsupported move-bundle rows reviewable and
fail-closed.

Actions:
- Ensure diagnostics distinguish RV64 consumer gaps from prepared producer
  gaps and destination fan-in blockers.
- Add focused negative coverage for representative rejected shapes when the
  current tests do not already protect them.
- Confirm no expectation, unsupported-marker, allowlist, timeout, runtime, or
  accounting changes are part of the slice.

Completion check:
- Reviewer can tell which residual rows belong to idea 610 and which should be
  split or left blocked.
- Backend subset proof passes after diagnostics and coverage changes.

### Step 5: Close-Readiness Review

Goal: decide whether idea 610 is complete, should continue with another
runbook, or should split remaining work.

Actions:
- Compare implementation movement against the source idea's acceptance
  criteria and reviewer reject signals.
- Verify multiple authorized move-bundle rows advanced and fail-closed rows are
  not weakened.
- Recommend close only if the source idea's RV64 consumer scope is satisfied.

Completion check:
- `todo.md` records close-readiness evidence and the exact validation result.
- If residual work remains outside idea 610, propose a separate `ideas/open/`
  initiative instead of expanding this runbook.
