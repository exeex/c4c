# RV64 Object Terminator Lowering Plan

Status: Active
Source Idea: ideas/open/674_rv64_object_terminator_lowering.md

## Purpose

Repair the row 176 RV64 object-emission failure where a prepared conditional
or fused pointer branch reaches `--codegen obj` and fails with
`unsupported_terminator_fragment`.

## Goal

Lower the proven prepared branch contract semantically in RV64 object emission,
while keeping row 256 `backend_riscv_object_emission` green and preserving the
accepted broader baseline shape.

## Core Rule

Do not repair row 176 with testcase-shaped matching, expectation churn, marker
changes, baseline accounting, or broad terminator rewrites disconnected from
the first proven prepared branch-lowering contract.

## Read First

- `ideas/open/674_rv64_object_terminator_lowering.md`
- `ideas/open/664_riscv_object_emission_internal_probe.md`
- `ideas/closed/673_post_664_full_suite_regression_probe.md`
- `build/agent_state/673_step1_regression_probe/summary.md`

## Current Target

- `backend_obj_runtime_rv64_indirect_store_postincrement_callee_contract`
- First known failure: `unsupported_terminator_fragment: BIR terminator requires unsupported RV64 object lowering`
- Guard surface: `backend_riscv_object_emission`

## Non-Goals

- Do not reopen row 139 `backend_cli_riscv64_pointer_global_local_publication`.
- Do not reopen the completed row 256 object-emission probe unless it regresses.
- Do not change tests, expectations, unsupported markers, allowlists, timeouts,
  runtime policy, or baseline accounting as a substitute for lowering support.
- Do not absorb unrelated RV64 runtime, local-memory, byval, AArch64, CLI, or
  dump-contract rows into this plan without a new lifecycle split.

## Working Model

Row 176's prepared dump succeeds and exposes branch stack-load authority for a
prepared conditional or fused pointer branch. The object-emission path reaches
terminator lowering and fails before clang link or QEMU runtime. The likely
owner is RV64 object terminator fragment generation, not prepared fact
production.

## Execution Rules

- Start each implementation packet by refreshing the first failing row and
  naming the exact terminator shape before editing code.
- Prefer the existing prepared/RV64 lowering helpers and diagnostics over a
  parallel special path.
- Keep malformed or unsupported terminator shapes fail-closed with precise
  diagnostics.
- Pair row 176 proof with row 256 guard proof for any code-changing packet.
- Use broader regression proof before lifecycle close.

## Ordered Steps

### Step 1: Refresh Row 176 Terminator Evidence

Goal: confirm the current first failure and collect the prepared branch facts
needed to choose the lowering contract.

Actions:

- Reproduce `backend_obj_runtime_rv64_indirect_store_postincrement_callee_contract`.
- Capture the failing `--codegen obj` diagnostic and the relevant prepared dump
  branch facts.
- Identify the exact terminator shape, source values, stack-load authorities,
  target labels, and comparison mode involved.
- Confirm `backend_riscv_object_emission` remains a guard row, not the active
  owner.

Completion check:

- `todo.md` names the first failing terminator contract and the implementation
  boundary to inspect next.

### Step 2: Select The Semantic Terminator-Lowering Boundary

Goal: choose the smallest general RV64 object-emission rule that can lower the
proven row 176 branch shape.

Actions:

- Inspect RV64 object terminator emission and nearby prepared branch lowering
  helpers.
- Decide whether the repair belongs in terminator fragment selection, branch
  stack-load materialization, compare lowering, label relocation, or a shared
  helper.
- Record any malformed or unsupported sibling shapes that must remain
  fail-closed.

Completion check:

- `todo.md` records the selected owner, nearby guard shapes, and the exact proof
  command for the implementation packet.

### Step 3: Implement The Focused Terminator Lowering

Goal: add semantic object lowering for the selected prepared branch contract.

Actions:

- Modify only the RV64 object-emission surfaces needed for the selected
  terminator rule.
- Reuse existing prepared facts and RV64 fragment helpers where possible.
- Preserve precise diagnostics for unsupported terminator shapes.
- Do not edit test expectations, markers, allowlists, runtime policy, or
  baseline files.

Completion check:

- The supervisor-selected focused proof shows row 176 improvement.
- `backend_riscv_object_emission` remains passing.
- `todo.md` records the before/after delta and any remaining first failure.

### Step 4: Prove Regression Safety And Close Readiness

Goal: establish whether the source idea is complete and safe to close.

Actions:

- Run the focused row 176 plus row 256 proof selected by the supervisor.
- Compare against the accepted 3386/3397 baseline or another supervisor-chosen
  close gate.
- If broader proof finds unrelated failures, split them into new ideas instead
  of expanding this plan.

Completion check:

- Row 176 no longer fails with `unsupported_terminator_fragment`.
- Row 256 stays green.
- Regression proof has no new failures for the chosen close scope.
- The plan owner can decide whether to close the source idea.
