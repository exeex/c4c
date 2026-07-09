# RV64 Call-Argument Frame-Slot Address Materialization Runbook

Status: Active
Source Idea: ideas/open/648_rv64_call_arg_frame_slot_address_materialization.md

## Purpose

Repair RV64 call-argument lowering for prepared call arguments whose selected
source is a local frame-slot address materialization.

## Goal

Lower `arg.source_selection=local_frame_address_materialization` into the ABI
argument register by computing the selected frame-slot address, not by copying a
stale register home.

## Core Rule

RV64 may use the frame-slot address path only when the prepared call plan
explicitly selects `local_frame_address_materialization`. Do not infer this
from source spelling, stack offsets, final assembly, testcase identity, or
diagnostic text.

## Read First

- `ideas/open/648_rv64_call_arg_frame_slot_address_materialization.md`
- `ideas/closed/638_rv64_string_label_pointer_runtime_object_correctness.md`
- `ideas/closed/630_string_constant_local_memory_policy.md`
- `ideas/closed/618_runtime_mismatch_ownership_investigation.md`

## Current Scope

- Representative integration surface: `src/20000722-1.c`.
- Prepared evidence should show `%lv._clit_` selected as
  `arg.source_selection=local_frame_address_materialization`.
- RV64 evidence currently lowers that argument through
  `reason=call_arg_register_to_register` and emits the stale `mv a0,s2` shape.
- The desired RV64 object/disassembly evidence should pass the selected local
  frame-slot address to the call, matching the semantic prepared source.

## Non-Goals

- Do not reopen string-constant local-memory admission or broaden
  `StringConstantLabelPointer` policy.
- Do not change generic runtime support, branch/control-flow lowering, stack
  layout, or ABI destination-register convention.
- Do not special-case `src/20000722-1.c`, `%lv._clit_`, `foo`, `s2`, or `a0`.
- Do not edit expectations, unsupported markers, allowlists, timeouts,
  runtime-comparison policy, or pass/fail accounting as progress.

## Working Model

The prepared call plan already owns the semantic source selection. RV64
call-argument lowering must consume that selection and materialize the selected
local frame-slot address into the ABI argument register. Missing, ambiguous, or
non-frame-slot call argument sources must continue to fail closed rather than
guessing from incidental layout or generated code.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Start with refreshed narrow diagnostics before editing lowering code.
- Add or update focused backend assertions before relying on the GCC torture
  integration row as proof.
- Preserve the old register-home-copy path for arguments whose prepared source
  is not `local_frame_address_materialization`.
- Escalate to review if the route drifts into local-memory policy, broad ABI
  rewrites, or named-case handling.

## Steps

### Step 1: Refresh The Call-Argument Source Evidence

Goal: Confirm the prepared source selection and current RV64 failure mode for
the representative row.

Actions:

- Run focused prepared-BIR, prepared call-plan, RV64 object, and disassembly
  diagnostics for `src/20000722-1.c`.
- Record the call argument, selected frame slot, ABI destination register, old
  register-home copy, and observable failure mode in `todo.md`.
- Confirm that the first owner is RV64 consumption of
  `arg.source_selection=local_frame_address_materialization`, not string-label
  pointer admission, stack layout, ABI convention, branch lowering, or runtime
  support.

Completion check:

- `todo.md` names the exact prepared fact shape and RV64 lowering site to
  inspect next, with no implementation packet selected from stale evidence.

### Step 2: Locate The RV64 Call-Argument Lowering Boundary

Goal: Find the narrow consumer path that maps prepared call-argument source
selection to emitted RV64 argument setup.

Actions:

- Locate the RV64 call lowering code that emits
  `reason=call_arg_register_to_register`.
- Trace how prepared call-plan source selections reach that code.
- Identify the smallest branch or helper needed for
  `local_frame_address_materialization`.
- Identify the fail-closed path for missing, ambiguous, or unsupported source
  selections.

Completion check:

- `todo.md` records the owned implementation surface, narrow test target, and
  negative behavior that must be preserved.

### Step 3: Add Focused Positive And Negative Coverage

Goal: Make the semantic boundary observable before or alongside implementation.

Actions:

- Add or update a focused backend assertion proving that
  `arg.source_selection=local_frame_address_materialization` does not lower
  through `call_arg_register_to_register`.
- Add or update negative coverage for missing, ambiguous, or non-frame-slot
  argument sources remaining fail-closed.
- Keep focused tests tied to prepared source selection, not to
  `src/20000722-1.c` identifiers.

Completion check:

- The focused test names the selected source path and would fail on the old
  stale register-home copy behavior.

### Step 4: Implement Frame-Slot Address Argument Materialization

Goal: Teach RV64 call lowering to materialize the selected local frame-slot
address into the ABI argument register.

Actions:

- Consume the explicit prepared
  `arg.source_selection=local_frame_address_materialization` fact.
- Emit the selected frame-slot address calculation into the argument register.
- Preserve existing register-to-register lowering for ordinary register-backed
  arguments.
- Keep missing, ambiguous, or non-frame-slot source selections rejected rather
  than guessed from stack offsets or source syntax.

Completion check:

- Focused positive and negative backend tests pass after a fresh build.

### Step 5: Prove The Representative Integration Row

Goal: Show that `src/20000722-1.c` advances through the stale `mv a0,s2`
failure mode.

Actions:

- Rerun the focused backend assertion and the RV64 GCC C torture backend path
  for `src/20000722-1.c`.
- Capture object/disassembly evidence showing the call receives the selected
  local frame-slot address.
- Record any remaining downstream owner in `todo.md` if the row advances but
  does not fully pass.

Completion check:

- `src/20000722-1.c` no longer fails because the call argument is copied from
  the stale register home, and proof logs identify the exact validation
  commands.

### Step 6: Run Broader Validation And Close Or Park

Goal: Decide whether the source idea is complete after the focused repair.

Actions:

- Run the supervisor-selected broader validation for the affected backend
  bucket after the focused proof is green.
- If the source idea acceptance criteria are satisfied, request plan-owner
  close with regression-guard proof.
- If a distinct downstream owner remains, record it in `todo.md` and request a
  lifecycle split or park decision rather than expanding this runbook.

Completion check:

- Lifecycle state either closes the source idea with passing guard proof or
  records a precise blocked or follow-up owner without broadening this idea.
