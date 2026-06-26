# RV64 Object Route Stack Frame And Parameter Home Edges Runbook

Status: Active
Source Idea: ideas/open/398_rv64_object_route_stack_frame_and_param_home_edges.md
Activated after: ideas/closed/397_rv64_object_route_move_bundle_target_shapes.md

## Purpose

Repair the current RV64 prepared-object route bucket where stack-frame,
callee-saved GPR, parameter-home, or related function-admission facts block
object emission.

## Goal

Classify and repair reusable ABI admission edges so prepared stack frames,
callee-saved save slots, GPR/FPR parameter homes, and the narrow current
variadic admission boundary lower through RV64 object emission using explicit
prepared facts.

## Core Rule

RV64 object emission may consume explicit prepared frame and home facts. It
must not reimplement the ABI classifier, fabricate missing homes, or infer
callee-saved/variadic state that the prepared contract did not publish.

## Read First

- `ideas/open/398_rv64_object_route_stack_frame_and_param_home_edges.md`
- `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`
- `ideas/closed/395_rv64_object_route_instruction_fragment_lowering.md`
- `ideas/closed/396_rv64_object_route_terminator_fragment_lowering_refresh.md`
- `ideas/closed/397_rv64_object_route_move_bundle_target_shapes.md`
- `tests/c/external/gcc_torture/src/930603-1.c`
- `tests/c/external/gcc_torture/src/20001017-1.c`
- `tests/c/external/gcc_torture/src/va-arg-21.c`
- Current RV64 gcc_torture backend probe artifacts under `build/agent_state/`
  and `build/rv64_gcc_c_torture_backend/`

## Current Targets

- Callee-saved GPR save-slot representative:
  `tests/c/external/gcc_torture/src/930603-1.c`
- Prepared stack-frame / parameter-home representative:
  `tests/c/external/gcc_torture/src/20001017-1.c`
- Variadic admission representative:
  `tests/c/external/gcc_torture/src/va-arg-21.c`
- Nearby same-bucket frame/home representatives selected by the supervisor
  from current RV64 gcc_torture backend artifacts.

## Non-Goals

- Do not reimplement the full ABI classifier in the object emitter.
- Do not reopen already-closed byval, sret, or va_list ideas unless current
  logs prove a new fact boundary.
- Do not treat semantic function-signature failures as this backend bucket.
- Do not fabricate stack-frame, callee-saved, parameter-home, or variadic
  facts that are absent from prepared output.
- Do not downgrade torture cases to unsupported, weaken allowlists, or use
  filename-specific branches.

## Working Model

The reopened 354 classification found 103 ABI admission failures:

- 66 `RV64 object route requires supported prepared callee-saved GPR save slots`
- 14 `RV64 object route requires a supported prepared stack frame`
- 22 `RV64 object route requires all parameters in supported GPR or prepared FPR register homes`
- 1 variadic function admission failure for missing `va_start` destination
  `va_list` address facts

The first packet should refresh the representative set and classify which
failures still reproduce in the current tree. Implementation should only begin
after the current frame/home/admission family is named and confirmed to have
explicit prepared facts for RV64 object emission to consume.

## Execution Rules

- Start with classification proof before implementation.
- Keep each implementation packet tied to one concrete frame, callee-saved
  save-slot, parameter-home, FPR-home, or variadic admission family.
- Inspect prepared dumps and object-route diagnostics before editing target
  emission.
- Preserve ABI placement, stack alignment, register bank, callee-saved save
  and restore semantics, parameter width, and variadic `va_list` state.
- If a representative lacks required prepared facts, stop and route that
  producer boundary instead of compensating in the RV64 emitter.
- Use the supervisor-selected proof command and record exact results in
  `todo.md`.
- If a repaired case object-compiles and links, include qemu comparison in the
  proof.
- Treat ABI misplacement, expectation rewrites, allowlist filtering, and
  named-case green proof as route failures.

## Step 1: Classify Current Stack Frame And Home Rejections

Goal: identify the concrete prepared frame, save-slot, parameter-home, or
variadic admission facts currently blocking RV64 object emission.

Actions:

- Reproduce or inspect the supervisor-selected RV64 gcc_torture backend probe
  for `930603-1.c`, `20001017-1.c`, `va-arg-21.c`, and nearby same-bucket
  frame/home cases.
- Dump or inspect prepared BIR for each representative and record stack-frame
  shape, callee-saved save slots, parameter homes, register banks, widths,
  FPR/GPR placement, variadic `va_start` destination facts, and function
  admission diagnostics.
- Map each rejection to the RV64 object-route code that emits the current
  stack-frame, callee-saved, parameter-home, or variadic admission diagnostic.
- Decide whether the first repair packet is a callee-saved save-slot,
  prepared stack-frame, GPR/FPR parameter-home, or producer missing-fact route.
- Route any missing prepared fact to lifecycle review instead of patching RV64
  object emission.

Completion check:

- `todo.md` records the concrete frame/home/admission family for the first
  executor packet, the representative set, and the exact
  supervisor-delegated proof command.
- Any non-398 residual is routed with precise evidence instead of patched in
  RV64 object emission.

## Step 2: Repair The First Valid Frame Or Home Family

Goal: add reusable RV64 object admission/lowering for the first classified
frame/home family when all required prepared facts are explicit.

Actions:

- Update the RV64 object route to consume the prepared frame, callee-saved,
  parameter-home, FPR-home, or variadic facts for the selected family.
- Preserve correct stack layout, save/restore behavior, ABI register bank,
  parameter width, and call/return semantics.
- Preserve existing diagnostics for unsupported or incomplete prepared facts.
- Add or update focused backend coverage where the repo has a matching test
  surface for the admission family.

Completion check:

- The selected representative no longer fails with the same stack-frame,
  callee-saved, parameter-home, or variadic admission diagnostic.
- Nearby same-family cases examined by the executor either advance together or
  are explicitly classified as separate owners.
- Existing backend tests for adjacent ABI, frame, stack, parameter, and call
  lowering remain green.

## Step 3: Prove Representatives And Residual Ownership

Goal: prove the frame/home repair advanced 398 without hiding runtime or
ownership failures.

Actions:

- Run the supervisor-selected narrow RV64 gcc_torture backend probe for the
  repaired representative and same-family additions.
- If a case object-compiles and links, inspect qemu comparison rather than
  stopping at compile success.
- Run the supervisor-selected backend CTest subset for the implementation
  slice.
- Classify any remaining failure as the same frame/home family, a distinct
  target-emission family, or a producer-fact gap that needs a separate source
  idea.

Completion check:

- `todo.md` records exact proof commands, results, and residual ownership.
- No expectation rewrites, unsupported downgrades, allowlist filtering,
  fabricated ABI facts, misplaced homes, or filename-specific fixes are used
  as acceptance evidence.
- The supervisor has enough evidence to continue with another 398 packet,
  request route review, or ask the plan owner for close/deactivation handling.
