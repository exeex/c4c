# RV64 Object Route Short-Circuit Call Argument Reload Runbook

Status: Active
Source Idea: ideas/open/380_rv64_object_route_short_circuit_call_argument_reload.md

## Purpose

Repair the RV64 object-route call-argument preservation or reload failure now
exposed by `src/20000112-1.c` after idea 379 fixed the first select/publication
owner.

Goal: prove and repair the first semantic incoming-argument preservation or
reload failure across repeated short-circuit RHS calls, then rerun
`src/20000112-1.c` to confirm it passes or advances to a distinct next owner.

## Core Rule

Use prepared call, argument, value-home, and object-route facts. Do not key
behavior on `src/20000112-1.c`, `special_format`, a specific `strchr` call,
exact C expression spelling, block labels, value ids, instruction indexes,
physical registers, object addresses, or log text.

## Read First

- `ideas/open/380_rv64_object_route_short_circuit_call_argument_reload.md`
- `build/rv64_gcc_c_torture_backend/src_20000112-1.c/case.log`
- `build/agent_state/379_step4_20000112.classification.txt`
- `build/agent_state/379_step4_20000112.c4c_o_objdump.txt`
- `build/agent_state/379_step4_20000112.c4c_bin_objdump.txt`
- `build/agent_state/379_step4_20000112.clang_bin_objdump.txt`
- `build/agent_state/379_step4_20000112.c4c_qemu_L_strace.err`
- `build/agent_state/379_step4_20000112.c4c_qemu_trace.log`
- `build/agent_state/379_step4_20000112.clang_qemu_L_strace.err`

## Current Targets

- Primary representative: `src/20000112-1.c`
- Failure class entering this plan: `RV64_BACKEND_RUNTIME_MISMATCH`
- Reported symptom from idea 379 handoff: the generated c4c binary enters a
  later short-circuit RHS and calls `strchr` with the string argument register
  holding NULL.
- Suspected owner from handoff evidence: incoming pointer argument home is
  lost, clobbered, or not reloaded before a later short-circuit RHS call after
  an earlier call clobbers argument registers.

## Non-Goals

- Do not reopen idea 379 select/publication lowering while its repaired
  behavior remains green.
- Do not add testcase-name, `special_format`, or `strchr`-specific handling.
- Do not repair broad register allocation, full ABI design, assembler
  replacement, byval aggregate homes, aggregate `va_arg`, globals, or strings
  unless the call-argument audit proves they are the direct owner.
- Do not weaken runtime expectations, external allowlists, supported-path
  contracts, or focused backend coverage.

## Working Model

- Treat this as an RV64 object-route incoming call-argument liveness/reload
  contract problem until evidence proves otherwise.
- First identify where the original incoming argument home stops being
  available for a later prepared call.
- Admit only a semantic rule based on prepared call, argument, and value-home
  facts.
- Keep nearby unsupported or ambiguous repeated-call shapes fail-closed.
- If `src/20000112-1.c` advances to a distinct owner after the repair, document
  that owner in `todo.md` instead of expanding this plan.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Keep implementation packets small and semantic.
- For each code-changing step, run the focused backend build/test proof chosen
  by the supervisor.
- Rerun `src/20000112-1.c` after the admitted repair.
- Escalate to review or plan-owner repair if the route requires broad
  register-allocation or ABI redesign.

## Steps

### Step 1: Audit Short-Circuit Call Argument Loss

Goal: identify the first semantic incoming-argument preservation or reload
failure for repeated short-circuit RHS calls in `src/20000112-1.c`.

Actions:
- Inspect the representative runtime failure using prepared call, argument,
  value-home, object-route, objdump, and qemu artifacts.
- Compare c4c and clang handling around the repeated RHS calls that reuse the
  incoming string pointer argument.
- Determine where the original argument home is lost, clobbered, or not
  reloaded before the later call.
- Record the semantic shape, required facts, involved value homes, and why the
  current object emission passes NULL.

Completion check:
- `todo.md` names the first semantic argument-preservation or reload shape and
  includes the proof artifacts or commands used to identify it.

### Step 2: Implement the First Semantic Reload Repair

Goal: preserve or reload the audited incoming argument for later prepared calls
without testcase-shaped matching.

Actions:
- Implement the narrow semantic repair in the responsible prepared/object-route
  lowering surface.
- Base the repair on prepared call, argument, and value-home facts, not
  physical-register coincidences or source testcase details.
- Preserve fail-closed behavior for unsupported or ambiguous repeated-call
  argument shapes.
- Keep idea 379's repaired select/publication behavior intact.

Completion check:
- Focused backend tests prove the accepted repeated-call incoming-pointer
  behavior and reject adjacent unsupported or ambiguous shapes.
- Existing focused backend coverage for idea 379's select/publication repair
  remains green.
- The supervisor-delegated focused build and test command passes.

### Step 3: Rerun `src/20000112-1.c`

Goal: confirm the representative result after the call-argument reload repair.

Actions:
- Rerun `src/20000112-1.c` through the RV64 GCC torture backend progress path
  using the supervisor-provided command.
- Inspect any remaining failure to decide whether it is still the same
  call-argument owner or a distinct next owner.

Completion check:
- The representative passes, or `todo.md` documents a distinct next owner with
  artifacts and a handoff recommendation.

### Step 4: Closure or Handoff Check

Goal: decide whether idea 380 is complete or needs another lifecycle split.

Actions:
- Verify the first call-argument preservation/reload failure from this source
  idea has been repaired or intentionally handed off as a distinct owner.
- Confirm focused backend coverage for the admitted repeated-call behavior
  still passes.
- Confirm idea 379's focused select/publication coverage still passes.
- Ask the plan owner to close idea 380 only if the source idea acceptance
  criteria are satisfied and regression guard can pass.

Completion check:
- Either the supervisor has enough evidence to request closure of idea 380, or
  `todo.md` clearly identifies the remaining blocker and why it is still in
  scope.
