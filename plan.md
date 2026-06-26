# Prepared I16 Same-Module Call Argument ABI Publication Runbook

Status: Active
Source Idea: ideas/open/407_prepared_i16_same_module_call_arg_abi_publication.md
Reopened from: ideas/closed/407_prepared_i16_same_module_call_arg_abi_publication.md
Supersedes active runbook from: ideas/open/395_rv64_object_route_instruction_fragment_lowering.md

## Purpose

Repair the producer-side prepared ABI publication gap where same-module `i16`
frame-slot call arguments still lack a target-consumable GPR argument
destination.

## Goal

Make same-module `i16` scalar call arguments publish complete prepared
call-plan destination facts before RV64 object emission consumes them.

## Core Rule

Fix the caller-side producer publication path. Do not teach RV64
`object_emission.cpp` to infer scalar argument registers, destination banks,
or ABI policy from source type, argument index, callee formal homes, or
testcase shape.

## Read First

- `ideas/open/407_prepared_i16_same_module_call_arg_abi_publication.md`
- `ideas/open/395_rv64_object_route_instruction_fragment_lowering.md`
- `ideas/closed/403_prepared_i16_formal_abi_publication.md`
- `build/agent_state/395_reclassify_step1_dumps/divmod-1.prepared.txt`
- `build/agent_state/395_reclassify_step1_probe.log`
- `tests/c/external/gcc_torture/src/divmod-1.c`
- The producer path that publishes same-module call argument value banks,
  destination placements, destination banks, and move reasons before RV64
  object emission

## Current Targets

- Primary representative: `tests/c/external/gcc_torture/src/divmod-1.c`.
- Live failing shape after clean rebuild: frame-slot same-module `i16` call
  arguments with `value_bank=gpr`, `dest_bank=none`, and move reason
  `call_arg_stack_to_stack`.
- Previously separated non-target: `src/20000223-1.c` passes the current RV64
  GCC torture backend probe and does not share this producer blocker.

## Non-Goals

- Do not implement RV64 opcode lowering under 395 while this producer fact is
  incomplete.
- Do not lower this by adding scalar argument ABI inference to
  `src/backend/riscv/rv64/object_emission.cpp`.
- Do not reopen the closed 403 incoming-formal repair unless fresh evidence
  proves direct `i16` formal ABI publication regressed.
- Do not redesign aggregate, byval, sret, variadic, or stack-passed argument
  handling.
- Do not rewrite gcc_torture expectations, change allowlists, or add
  filename-specific handling for `divmod-1.c`.

## Working Model

The previous 407 close was premature. The current prepared dump no longer
matches the earliest fully no-bank shape, but it still fails the same producer
contract: same-module `i16` frame-slot call arguments do not publish a complete
target-consumable destination. `value_bank=gpr` is present, but
`dest_bank=none` and `call_arg_stack_to_stack` show the call-plan destination
publication is still incomplete.

RV64 object emission should not infer the missing destination. The producer
must publish explicit `dest_placement=gpr:call_argument#N/w1`,
`dest_reg=aN`, and `dest_bank=gpr` facts for ABI-register arguments, while
preserving legitimate stack-passed and unsupported shapes.

## Execution Rules

- Keep implementation packets scoped to same-module scalar `i16`
  call-argument publication from frame-slot sources.
- Inspect prepared dumps before changing code and record the exact call-plan
  facts in `todo.md`.
- Prefer repairing the existing call-ABI/register-index authority over adding a
  parallel classifier.
- Preserve existing behavior for `i1`, `i8`, `i32`, `i64`, pointer,
  aggregate, variadic, byval, and stack-passed calls unless local proof
  requires a narrow supporting adjustment.
- Use the supervisor-selected proof command and record exact results in
  `todo.md`.
- Treat diagnostic-only churn, expectation rewrites, RV64 consumer inference,
  and single-case green proof as insufficient progress.

## Step 1: Reconfirm Current Frame-Slot I16 Destination Gap

Goal: identify the exact producer path and facts that still leave same-module
`i16` frame-slot call arguments with incomplete destination publication.

Actions:

- Inspect the fresh prepared dump for `src/divmod-1.c` and name every
  same-module `i16` frame-slot call argument with `value_bank`, source
  encoding, source slot, destination placement, destination register,
  destination bank, and move reason.
- Compare against supported same-module scalar argument widths that publish
  complete GPR destinations.
- Trace the producer path that creates call-plan argument value banks,
  destination placements, destination banks, and before-call move reasons.
- Decide the first implementation packet owner and the exact helper/function
  family to repair.

Completion check:

- `todo.md` names the concrete producer function or helper family to repair,
  the observed incomplete facts, the contrasting supported facts, and the
  supervisor-delegated proof command.
- If the failure is not this call-argument ABI publication gap, stop and
  request lifecycle review instead of patching RV64 object emission.

## Step 2: Publish Complete GPR Destinations For Frame-Slot I16 Arguments

Goal: repair the producer path so same-module `i16` frame-slot scalar
arguments publish a complete target-consumable GPR argument destination when
the ABI uses a GPR.

Actions:

- Extend the selected producer path for `i16` frame-slot sources using the same
  semantic pattern as adjacent supported scalar integer widths.
- Ensure prepared call-plan facts include `dest_placement`, `dest_reg`, and
  `dest_bank=gpr` for ABI-register arguments.
- Replace or avoid stale `call_arg_stack_to_stack` ownership when a real GPR
  call-argument destination is required.
- Preserve precise diagnostics for unsupported argument shapes that still lack
  legitimate prepared facts.
- Add or update focused backend or prepared tests if a local test surface
  already exists for same-module scalar call-argument publication.

Completion check:

- Fresh `src/divmod-1.c` prepared dump no longer shows same-module `i16`
  frame-slot call arguments with `value_bank=gpr`, `dest_bank=none`, and
  `call_arg_stack_to_stack` as the blocking shape.
- Existing supported scalar call-argument cases remain supported.
- RV64 object emission does not gain scalar call-argument ABI inference.

## Step 3: Prove Divmod And Route Residuals

Goal: prove the producer repair removes the `src/divmod-1.c` call-argument
publication blocker without crossing the prepared-object consumer boundary.

Actions:

- Run the supervisor-selected build and narrow RV64 gcc_torture backend proof
  for `src/divmod-1.c` and any minimized same-module `i16` artifacts.
- Inspect fresh prepared dumps for `div2`, `div4`, `mod2`, and `mod4`
  callsites.
- Run the supervisor-selected backend CTest subset for the implementation
  slice.
- Classify any newly exposed later failure as inside this idea, back in 395,
  owned by another open idea, or requiring a new producer-side split.

Completion check:

- `todo.md` records exact proof results and states whether this idea is
  complete, needs another same-module `i16` call-argument packet, or should be
  routed to another owner.
- No expectation rewrites, unsupported downgrades, allowlist filtering, or
  filename-specific fixes are used as acceptance evidence.
