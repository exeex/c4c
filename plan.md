# Prepared I16 Same-Module Call Argument ABI Publication Runbook

Status: Active
Source Idea: ideas/open/407_prepared_i16_same_module_call_arg_abi_publication.md
Supersedes active runbook from: ideas/open/395_rv64_object_route_instruction_fragment_lowering.md

## Purpose

Repair the producer-side prepared ABI publication gap where same-module `i16`
scalar call arguments remain no-bank frame-slot arguments even though the ABI
requires a GPR argument destination.

## Goal

Make same-module `i16` scalar call arguments publish target-consumable prepared
call-plan facts before RV64 object emission.

## Core Rule

Fix the caller-side producer publication path. Do not teach RV64
`object_emission.cpp` to infer scalar argument registers, destination banks, or
ABI policy from source type, argument index, callee formal homes, or testcase
shape.

## Read First

- `ideas/open/407_prepared_i16_same_module_call_arg_abi_publication.md`
- `ideas/closed/403_prepared_i16_formal_abi_publication.md`
- `ideas/open/395_rv64_object_route_instruction_fragment_lowering.md`
- `build/agent_state/395_step4_divmod_residual_prepared.txt`
- `build/agent_state/395_step4_divmod_residual_i8_call.prepared.txt`
- `build/agent_state/395_step4_divmod_residual_i16_call.prepared.txt`
- The producer path that plans or repairs same-module call arguments before
  RV64 object emission

## Current Targets

- Primary representative: `tests/c/external/gcc_torture/src/divmod-1.c`.
- Minimized failing artifact:
  `build/agent_state/395_step4_divmod_residual_i16_call.c`.
- Contrasting supported artifact:
  `build/agent_state/395_step4_divmod_residual_i8_call.c`.
- Prepared failing shape: `wrapper_kind=same_module`, `arg index=0`,
  `value_bank=none`, `source_encoding=frame_slot`, `dest_bank=none`, move
  reason `call_arg_stack_to_stack`.

## Non-Goals

- Do not lower this by adding scalar argument ABI inference to
  `src/backend/riscv/rv64/object_emission.cpp`.
- Do not reopen the closed 403 incoming-formal repair unless fresh evidence
  proves that direct `i16` formal ABI publication regressed.
- Do not redesign aggregate, byval, sret, variadic, or stack-passed argument
  handling.
- Do not treat unrelated 395 instruction-fragment lowering, 397 move-bundle,
  or 398 parameter-home failures as part of this idea.
- Do not rewrite gcc_torture expectations, change allowlists, or add
  filename-specific handling for `divmod-1.c`.

## Working Model

The callee-side `i16` formal already has a register home. The remaining failure
is that the caller-side same-module argument is published as a frame-slot
source with no value bank or destination bank, leaving target emission without
an explicit scalar argument destination. The producer should publish the same
kind of target-consumable scalar argument facts that make the minimized `i8`
same-module case compile.

If investigation proves the no-bank shape is intentional and requires a
separate move-bundle or stack-home owner, stop and route that boundary instead
of expanding RV64 object emission.

## Execution Rules

- Keep each implementation packet scoped to same-module scalar `i16`
  call-argument publication.
- Inspect prepared dumps before changing code and record the exact call-plan
  facts in `todo.md`.
- Prefer extending existing scalar-width handling near the producer call-ABI
  publication path over adding a parallel classifier.
- Preserve existing behavior for `i1`, `i8`, `i32`, `i64`, pointer, aggregate,
  variadic, byval, and stack-passed calls unless local proof requires a narrow
  supporting adjustment.
- Use the supervisor-selected proof command and record exact results in
  `todo.md`.
- Treat diagnostic-only churn, expectation rewrites, and single-case green
  proof as insufficient progress.

## Step 1: Locate The Caller-Side Publication Gap

Goal: identify the producer code that publishes same-module scalar call
arguments and explain why `i8` gets target-consumable facts while `i16` remains
no-bank.

Actions:

- Inspect the prepared dumps for the full `divmod-1.c` case, the minimized
  failing `i16` artifact, and the contrasting `i8` artifact.
- Trace the producer path that creates `wrapper_kind=same_module` call plans,
  argument value banks, destination banks, and before-call move reasons.
- Compare scalar-width handling against the closed 403 formal repair so any
  shared helper can be reused without conflating formal and call-argument
  ownership.
- Decide whether the first implementation packet belongs in direct
  call-argument ABI publication, move-bundle production, or another producer
  boundary.

Completion check:

- `todo.md` names the exact producer function or helper family to repair, the
  observed `i8` versus `i16` fact delta, and the supervisor-delegated proof
  command.
- If the failure is not a call-argument ABI publication gap, stop and request
  lifecycle review instead of patching RV64 object emission.

## Step 2: Publish Same-Module I16 Argument ABI Facts

Goal: repair the producer path so same-module `i16` scalar arguments publish a
target-consumable argument destination when the ABI uses a GPR.

Actions:

- Extend the selected producer path for `i16` using the same semantic pattern
  as adjacent supported scalar integer widths.
- Ensure frame-slot sources are converted or published through explicit
  prepared facts before the call reaches RV64 object emission.
- Preserve precise diagnostics for unsupported argument shapes that still lack
  legitimate prepared facts.
- Add or update focused backend or prepared tests if a local test surface
  already exists for same-module scalar call-argument publication.

Completion check:

- The minimized `i16` same-module artifact no longer reaches object emission
  with `value_bank=none`, `source_encoding=frame_slot`, `dest_bank=none`, and
  `call_arg_stack_to_stack` as the blocking shape.
- Existing supported scalar call-argument cases, including the minimized `i8`
  artifact, remain supported.

## Step 3: Prove Divmod And Check Residual Ownership

Goal: prove the repair removes the `src/divmod-1.c` blocker without crossing
the prepared-object consumer boundary.

Actions:

- Run the supervisor-selected build and narrow RV64 gcc_torture backend proof
  for `src/divmod-1.c` and the minimized artifacts.
- Inspect fresh prepared dumps or diagnostics for `div2`, `div4`, `mod2`, and
  `mod4` callsites.
- Classify any newly exposed later failure as inside this idea, back in 395,
  owned by another open idea, or requiring a new producer-side split.

Completion check:

- `todo.md` records exact proof results and states whether this idea is
  complete, needs another same-module `i16` call-argument packet, or should be
  routed to another owner.
- RV64 object emission did not gain scalar call-argument ABI inference.

## Step 4: Close Or Route Remaining Scope

Goal: finish the producer-side call-argument publication idea cleanly.

Actions:

- Confirm the source idea acceptance criteria are satisfied or document the
  remaining producer boundary that prevents closure.
- If complete, prepare for close-gate regression guard through the plan owner.
- If blocked or split, preserve durable notes in the correct open idea and do
  not silently fold unrelated work into this runbook.

Completion check:

- The plan owner can either close this source idea with regression proof or
  switch/deactivate it with precise residual ownership.
