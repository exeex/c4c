# RV64 Object Route Residual Call Argument Prior-Preservation Runbook

Status: Active
Source Idea: ideas/open/380_rv64_object_route_short_circuit_call_argument_reload.md

## Purpose

Continue idea 380 after the first repair admitted one GPR callee-saved
`PriorPreservation` shape but did not resolve the representative
`src/20000112-1.c` crash.

Goal: repair the remaining semantic owner that lets the original incoming
`fmt` argument become unavailable before a later prepared call, then rerun the
representative to confirm it passes or advances to a distinct owner.

## Core Rule

Use prepared call, argument, value-home, prior-preservation, and object-route
facts. Do not key behavior on `src/20000112-1.c`, `special_format`, a specific
`strchr` call, exact C expression spelling, block labels, value ids,
instruction indexes, physical registers, object addresses, or log text.

## Read First

- `ideas/open/380_rv64_object_route_short_circuit_call_argument_reload.md`
- `todo.md`
- `build/agent_state/380_step4_20000112.classification.txt`
- `build/agent_state/380_step4_20000112.c4c_trace_tail.txt`
- `build/agent_state/380_step4_20000112.c4c_qemu_L_strace.err`
- `build/agent_state/380_step4_20000112.clang_qemu_L_strace.err`
- `build/agent_state/380_step4_20000112.c4c_o_objdump.txt`
- `build/agent_state/380_step4_20000112.clang_bin_objdump.txt`
- `build/rv64_gcc_c_torture_backend/src_20000112-1.c/case.log`

## Current Targets

- Primary representative: `src/20000112-1.c`
- Failure class entering this pass: `RV64_BACKEND_RUNTIME_MISMATCH`
- Residual symptom: after `strchr(fmt, '*')` returns `NULL` for `"ee"`, c4c
  reaches the next prepared call with `a0 == NULL` instead of preserving or
  reloading the original `fmt` pointer for the later `strchr(fmt, 'V')`.
- Current owner: still the idea 380 call-argument reload /
  prior-preservation owner family, not a separate runtime-crash initiative.

## Non-Goals

- Do not create a separate runtime-crash source idea for the current evidence.
- Do not reopen idea 379 select/publication lowering while its repaired
  behavior remains green.
- Do not add testcase-name, `special_format`, or `strchr`-specific handling.
- Do not broaden the route into full register allocation, full ABI redesign,
  assembler replacement, byval aggregate homes, aggregate `va_arg`, globals,
  or strings unless the residual audit proves the current prepared-call and
  value-home facts cannot express the repair.
- Do not weaken runtime expectations, external allowlists, supported-path
  contracts, or focused backend coverage.

## Working Model

- The original incoming argument must remain available across one RHS call that
  clobbers argument registers and produces a `NULL` result in the same call
  argument register needed by a later RHS call.
- The prior Step 2 repair accepted one GPR callee-saved `PriorPreservation`
  shape, but that acceptance was not sufficient for the representative.
- The next packet should identify whether the missing fact is the original
  incoming-argument home, the chosen prior-preservation source, the reload
  point before prepared call emission, or rejection of a currently unsupported
  multi-call shape.
- If the representative advances to a truly distinct owner after this repair,
  document that owner in `todo.md` instead of expanding this plan.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Keep implementation packets small and semantic.
- For each code-changing step, run the focused backend build/test proof chosen
  by the supervisor.
- Preserve fail-closed behavior for unsupported or ambiguous repeated-call
  argument shapes.
- Rerun `src/20000112-1.c` after the admitted residual repair.
- Escalate to reviewer or plan-owner repair if the route requires broad
  register-allocation or ABI redesign.

## Steps

### Step 1: Audit Residual Prior-Preservation Gap

Goal: identify why the first `PriorPreservation` repair does not keep the
original incoming `fmt` pointer available for the later prepared call.

Actions:
- Inspect the residual Step 4 artifacts and the current object-route emission
  around the first failed `strchr` return and the following prepared call.
- Compare the value homes for the original incoming argument, the first call
  result, and the later call argument.
- Determine whether the missing rule is prior source selection, home
  publication, reload placement, call-clobber invalidation, or an unsupported
  shape that must stay rejected.
- Record the smallest semantic shape that must be admitted and the adjacent
  shapes that must remain fail-closed.

Completion check:
- `todo.md` names the residual call-argument prior-preservation shape, the
  missing fact or lowering rule, and the artifacts or commands used to identify
  it.

### Step 2: Implement Residual Semantic Repair

Goal: preserve or reload the original incoming argument for the later prepared
call without testcase-shaped matching.

Actions:
- Implement the narrow semantic repair in the responsible prepared/object-route
  lowering surface.
- Base the repair on prepared call, argument, value-home, and
  prior-preservation facts, not physical-register coincidences or source
  testcase details.
- Ensure a call result held in the same outgoing argument register does not
  masquerade as the original incoming argument for a later call.
- Keep idea 379's repaired select/publication behavior intact.

Completion check:
- Focused backend tests prove the admitted residual repeated-call argument
  behavior and reject adjacent unsupported or ambiguous shapes.
- Existing focused backend coverage for the prior idea 380 repair and idea
  379's select/publication repair remains green.
- The supervisor-delegated focused build and test command passes.

### Step 3: Rerun `src/20000112-1.c`

Goal: confirm the representative result after the residual prior-preservation
repair.

Actions:
- Rerun `src/20000112-1.c` through the RV64 GCC torture backend progress path
  using the supervisor-provided command.
- Inspect any remaining failure to decide whether it is still the same
  call-argument owner or a distinct next owner.
- If it is the same owner, record the remaining reload/preservation gap in
  `todo.md` for another bounded executor packet.

Completion check:
- The representative passes, or `todo.md` documents a distinct next owner with
  artifacts and a handoff recommendation, or `todo.md` documents the still
  in-scope residual reload gap that needs another plan-owner review.

### Step 4: Closure or Handoff Check

Goal: decide whether idea 380 is complete after the residual repair.

Actions:
- Verify the source idea's call-argument preservation/reload acceptance
  criteria are satisfied.
- Confirm focused backend coverage for the admitted repeated-call behavior
  still passes.
- Confirm idea 379's focused select/publication coverage still passes.
- Ask the plan owner to close idea 380 only if the source idea acceptance
  criteria are satisfied and regression guard can pass.

Completion check:
- Either the supervisor has enough evidence to request closure of idea 380, or
  `todo.md` clearly identifies the remaining blocker and why it is still in
  scope.
