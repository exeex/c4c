# RV64 Va Start Stack-Backed Destination Runbook

Status: Active
Source Idea: ideas/open/582_rv64_va_start_stack_backed_destination.md

## Purpose

Repair or narrowly diagnose RV64 `va_start` helper lowering when the prepared
destination `va_list` address is stack-backed instead of already materialized
in a prepared GPR home.

## Goal

Advance `src/va-arg-21.c` past the current `unsupported_variadic_helper_lowering`
owner for stack-backed `dst_va_list_addr`, or replace it with a narrower
destination-address materialization diagnostic.

## Core Rule

Handle the semantic `va_start` helper operand shape; do not match
`src/va-arg-21.c`, libc declarations, or f128-looking surrounding source text.

## Read First

- `ideas/open/582_rv64_va_start_stack_backed_destination.md`
- `build/agent_state/550_rv64_scalar_fpr_residual_salvage/step2/summary.md`
- `build/agent_state/550_rv64_scalar_fpr_residual_salvage/step3/owners.tsv`
- `build/agent_state/550_rv64_scalar_fpr_residual_salvage/step3/summary.md`
- RV64 variadic helper lowering code and focused backend object-emission tests

## Current Scope

- RV64 `va_start` helper lowering for prepared `dst_va_list_addr` operands
  whose destination address lives in stack slots.
- Address materialization into a usable helper operand, or a precise
  fail-closed diagnostic for unsupported destination-address homes.
- Focused helper coverage for the semantic stack-backed destination shape.
- Representative route proof for `src/va-arg-21.c`.

## Non-Goals

- Scalar compare publication, ordinary floating-cast lowering, F128, long
  double, or soft-float helper implementation.
- Broad call ABI, varargs, or libc-header rewrites outside the `va_start`
  destination-address boundary.
- Reclassifying the row as scalar/FPR work because nearby declarations mention
  f128 prototypes.
- Expectation rewrites, unsupported-marker changes, or diagnostic-only
  relabeling claimed as capability progress.

## Working Model

The salvage evidence classified `src/va-arg-21.c` under a variadic helper
owner. The live shape is not f128 carrier work: prepared case-local f128
carrier/helper sections are empty, while helper operands record stack-backed
`dst_va_list_addr` values. The likely implementation surface is RV64 helper
destination-address materialization before or inside prepared helper emission.

## Execution Rules

- Keep changes inside the RV64 variadic helper lowering boundary unless Step 1
  proves a narrower owner.
- Add tests that construct the semantic helper operand shape without filename,
  function, block, or value-name shortcuts.
- Preserve fail-closed behavior for helper operands whose homes or types are
  not lowerable.
- Use `todo.md` for packet progress and proof notes; do not rewrite this
  runbook for routine executor updates.
- Backend proof must include a fresh build plus the focused RV64 backend bucket
  touched by the implementation. Escalate to `^backend_` before closure.

## Step 1: Reproduce Va Start Destination Owner

Goal: Confirm the current owner and exact prepared helper operand shape for the
representative row.

Actions:
- Build `c4cll`.
- Regenerate prepared dumps and RV64 object-route logs for `src/va-arg-21.c`.
- Record the helper operand homes, the `dst_va_list_addr` stack-slot evidence,
  and whether f128 carrier/helper sections remain empty.
- Identify the code path that reports `unsupported_variadic_helper_lowering`.

Completion check:
- `todo.md` records the current diagnostic, route coordinates when available,
  prepared helper facts, and a focused proof command/log.

## Step 2: Add Focused Helper Coverage

Goal: Pin the semantic stack-backed `va_start` destination shape in backend
tests before changing lowering behavior.

Actions:
- Add or adjust a focused RV64 object-emission test that builds the helper
  operand shape directly.
- Cover the retained unsupported path precisely if the current implementation
  cannot yet materialize the destination address.
- Avoid expectations tied to `src/va-arg-21.c` names or source text.

Completion check:
- The focused backend test fails for the intended owner before repair or
  passes with a precise fail-closed diagnostic contract, and `todo.md` records
  the proof.

## Step 3: Repair Destination-Address Materialization

Goal: Materialize stack-backed `dst_va_list_addr` operands for RV64 `va_start`
helper lowering, or prove and encode a narrower fail-closed diagnostic.

Actions:
- Locate the prepared helper emission path that consumes `dst_va_list_addr`.
- Materialize supported stack-backed destination addresses through the existing
  RV64 address/home helper machinery.
- Keep unsupported helper operand forms fail-closed with a diagnostic specific
  to destination-address materialization.
- Keep the focused helper test semantic, not representative-shaped.

Completion check:
- Focused RV64 backend coverage passes and proves either helper support for the
  stack-backed destination shape or the narrower diagnostic.

## Step 4: Representative Route Proof

Goal: Prove the retained representative advances past the old helper owner or
lands on the intended narrower owner.

Actions:
- Rebuild `c4cll`.
- Rerun prepared dumps and RV64 object-route logs for `src/va-arg-21.c`.
- Compare against Step 1 and record the new owner, downstream diagnostic, or
  successful object emission state.
- Confirm the proof does not depend on scalar compare, floating-cast, or f128
  changes.

Completion check:
- `todo.md` records that `src/va-arg-21.c` no longer stops at the old
  `unsupported_variadic_helper_lowering` reason, or records a narrower
  destination-address owner with concrete evidence.

## Step 5: Backend Closure Readiness

Goal: Establish that the idea is ready for lifecycle closure evaluation.

Actions:
- Run the focused backend object-emission test.
- Run the broader backend subset chosen by the supervisor, normally
  `ctest --test-dir build -j --output-on-failure -R '^backend_'`.
- Confirm the focused coverage, representative route proof, and source idea
  acceptance criteria are all satisfied.

Completion check:
- `todo.md` records fresh backend proof and explicitly states whether the
  source idea is ready for plan-owner closure evaluation.
