# Prepared `va_start` Destination Address Helper Operand Publication Runbook

Status: Active
Source Idea: ideas/open/408_prepared_va_start_destination_address_helper_operand_publication.md
Activated after: ideas/closed/398_rv64_object_route_stack_frame_and_param_home_edges.md

## Purpose

Repair the producer-side prepared metadata gap where supportable RV64
`va_start` helper calls still lack an explicit destination `va_list` address
helper operand fact.

## Goal

Publish `helper_operand_homes.va_start.destination_va_list_address` from the
prepared producer authority so RV64 helper lowering can consume explicit facts
instead of inferring the destination address from instruction or stack shape.

## Core Rule

Prepared producers must publish the `va_start` destination `va_list` address
when supportable. RV64 object emission must not reconstruct that address from
source text, BIR instruction shape, frame layout, or testcase identity.

## Read First

- `ideas/open/408_prepared_va_start_destination_address_helper_operand_publication.md`
- `ideas/closed/398_rv64_object_route_stack_frame_and_param_home_edges.md`
- `ideas/closed/389_rv64_va_start_destination_va_list_address_publication.md`
- `ideas/closed/366_rv64_va_start_destination_va_list_gpr_home.md`
- `tests/c/external/gcc_torture/src/va-arg-21.c`
- Current prepared dump artifacts under
  `build/agent_state/398_step1_frame_home_refresh/`

## Current Targets

- Primary representative: `tests/c/external/gcc_torture/src/va-arg-21.c`
- Focused backend/prepared tests that cover variadic entry, `va_start`
  helper operand homes, prepared printing, frame-stack-call facts, and RV64
  object emission.

## Non-Goals

- Do not reopen generic RV64 stack-frame, callee-saved save-slot, or
  parameter-home lowering from idea 398.
- Do not reopen closed target-side `va_start` helper lowering unless fresh
  proof shows that exact target failure regressed.
- Do not teach RV64 object emission to infer missing helper operand homes.
- Do not broaden into scalar `va_arg`, aggregate `va_arg`, `va_copy`, or
  external variadic call support.
- Do not downgrade expectations, filter allowlists, or use testcase-specific
  branches.

## Working Model

Idea 398 proved that the old RV64 object-route frame/home diagnostics are
stale or passing. Its fresh `va-arg-21.c` prepared dump still reports:

```text
missing fact=helper_operand_homes.va_start.destination_va_list_address
```

The same dump contains the surrounding variadic entry plan, `va_list` layout,
overflow-area state, incoming variadic GPR publications, and `llvm.va_start.p0`
call sites. The first packet should identify which producer owns the missing
helper operand home and confirm the supportable destination-address shape.

## Execution Rules

- Start with classification proof from the current `va-arg-21.c` prepared
  dump before implementation.
- Keep the repair in producer/prepared publication code unless evidence proves
  the consumer contract itself is wrong.
- Preserve separate facts for destination `va_list` storage and destination
  `va_list` address.
- Preserve precise unsupported diagnostics for unsupported or ambiguous
  destination address shapes.
- Use the supervisor-selected proof command and record exact results in
  `todo.md`.
- Treat target-emitter inference, diagnostic-only churn, expectation rewrites,
  and named-case green proof as route failures.

## Step 1: Classify The Missing Helper Operand Publication

Goal: identify the producer authority and exact supportable destination-address
shape for `va_start` helper operand publication.

Actions:

- Reproduce or inspect the fresh prepared dump for `va-arg-21.c`.
- Record the destination `va_list` object, address-producing facts,
  frame-slot/register homes, `llvm.va_start.p0` call operands, variadic entry
  state, and the missing helper operand line.
- Locate the producer path that emits `helper_operand_homes.va_start.*`
  metadata and decide why `destination_va_list_address` is absent.
- Decide whether the first repair packet publishes a frame-slot-backed
  destination address, a register-backed destination address, or routes a
  broader producer contract gap.

Completion check:

- `todo.md` records the exact missing producer fact, the supportable
  destination-address shape, the intended producer file/function area, and the
  supervisor-delegated proof command.
- Any non-408 residual is routed with precise evidence instead of patched in
  RV64 object emission.

## Step 2: Publish The Supported Destination Address Fact

Goal: make prepared output publish
`helper_operand_homes.va_start.destination_va_list_address` for the first
supportable destination-address shape.

Actions:

- Update the producer/prepared publication path to emit the explicit helper
  operand home.
- Keep destination storage and destination address metadata distinct.
- Preserve fail-closed behavior for unsupported or ambiguous shapes.
- Add or update focused backend/prepared coverage for the supported and
  unsupported helper operand publication paths.

Completion check:

- `va-arg-21.c` no longer reports missing
  `helper_operand_homes.va_start.destination_va_list_address` for the
  supportable shape.
- Focused tests prove the fact is present and unsupported adjacent shapes are
  still precise.

## Step 3: Prove Representative And Residual Ownership

Goal: prove the producer repair advanced 408 without hiding target-side or
runtime residuals.

Actions:

- Run the supervisor-selected `va-arg-21.c` object-route compile plus prepared
  dump proof.
- Run the supervisor-selected backend CTest subset.
- If the case advances to a new object-route, link, qemu, or runtime mismatch
  boundary, classify it separately instead of absorbing it into 408.

Completion check:

- `todo.md` records exact proof commands, results, and residual ownership.
- No expectation rewrites, unsupported downgrades, allowlist filtering,
  target-emitter inference, or filename-specific fixes are used as acceptance
  evidence.
- The supervisor has enough evidence to continue, request route review, or ask
  the plan owner for close/deactivation handling.
