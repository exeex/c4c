# RV64 Va List Post-Repair Runtime Boundary Runbook

Status: Active
Source Idea: ideas/open/392_rv64_va_list_expression_call_argument_value_publication.md
Supersedes: continuation runbook exhausted after Step 4; idea 392 remains open because `va-arg-13.c` still aborts after the load-local publication repair.

## Purpose

Continue idea 392 after the explicit `call_argument_value_publications` route
and the load-local payload rewrite were repaired. Focused backend coverage now
passes, but the representative still reaches `[RV64_BACKEND_RUNTIME_MISMATCH]`
with `c4c_exit=Subprocess aborted`.

## Goal

Trace the post-repair RV64 object/runtime state for `va-arg-13.c` and decide
whether the remaining abort is still the `va_list` call-argument payload
publication boundary, a nearby object-ABI state bug required by idea 392, or a
later boundary that must be split into a separate idea.

## Core Rule

Start from emitted object/runtime evidence after commit `9fb88adc`. Do not
assume the focused backend proof describes the representative runtime state,
and do not fix the abort by matching `va-arg-13.c`, `test`, `dummy`, literal
registers, stack offsets, or the abort branch.

## Read First

- `ideas/open/392_rv64_va_list_expression_call_argument_value_publication.md`
- `todo.md`
- `test_after.log`
- `build/agent_state/392_cont_step1_va-arg-13.analysis.log`
- `build/agent_state/392_cont_step1_va-arg-13.prepared.log`
- `build/agent_state/392_cont_step2_backend-selection.analysis.log`
- `build/agent_state/392_cont_step2_va-arg-13.trace-mir.log`
- `build/agent_state/392_cont_step4_va-arg-13.case.log`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`

## Current Targets

- Representative: `tests/c/external/gcc_torture/src/va-arg-13.c`
- Post-repair RV64 disassembly and runtime trace for `test` and `dummy`
- Caller-side argument object contents before each `dummy` call
- Callee-side `dummy` entry state and `va_arg` load path
- Object/ABI state that decides whether the remaining abort belongs to idea
  392 or a later split

## Non-Goals

- Do not reopen RV64 variadic prologue save-area publication from idea 391
  unless post-repair evidence proves the save area is no longer populated.
- Do not reopen `va_start` destination-address materialization from idea 389
  unless post-repair evidence proves the local `va_list` payload is not
  initialized.
- Do not weaken the explicit prepared publication fact guards added for idea
  392.
- Do not redesign generic variadic, aggregate, call ABI, or `va_arg` lowering.
- Do not downgrade expectations, suppress `abort()`, or mark the
  representative unsupported as progress.

## Working Model

Earlier continuation steps established:

- The representative has explicit `call_argument_value_publications` for both
  `dummy` calls.
- The backend selected the publication route but rewrote the payload from the
  load-local result to the local `va_list` storage object.
- Commit `9fb88adc` repaired that load-local rewrite and added focused backend
  coverage.
- Step 4 then reproved backend coverage but `va-arg-13.c` still aborted.

The next boundary is no longer allowed to assume the old `mv t1,s1` /
argument-object overwrite still exists. It must capture fresh post-repair
object evidence and classify the exact runtime state that reaches `abort()`.

## Execution Rules

- Capture fresh post-repair disassembly/runtime evidence before editing code.
- Keep these states separate: source `va_list` storage address, initialized
  save-area pointer payload, caller argument object address, caller argument
  object contents, callee parameter pointer, and callee `va_arg` loaded value.
- If the caller still passes the wrong payload, keep the repair inside idea
  392.
- If the caller payload is correct but `dummy` misinterprets object/ABI state,
  decide whether that behavior is required for idea 392 acceptance or should
  be split as a later idea.
- Preserve focused backend proof and fail-closed variants while adding any new
  representative-shaped coverage.

## Steps

### Step 1: Capture Post-Repair Runtime State

Goal: produce fresh evidence for the representative after the load-local
payload repair.

Primary target: `va-arg-13.c` object disassembly and qemu/runtime trace around
both `dummy` calls.

Actions:

- Regenerate the representative object, disassembly, and runtime logs under
  `build/agent_state/392_postrepair_step1_*`.
- Inspect the stores that populate the first and second caller argument
  objects immediately before `dummy`.
- Record whether the old local-storage-address overwrite is gone.
- Capture the values/addresses at `dummy` entry that determine the subsequent
  `va_arg` load and abort.
- Record the exact post-repair state in `todo.md`.

Completion check: `todo.md` states the caller argument object contents and
callee entry state for both `dummy` calls using fresh post-repair logs.

### Step 2: Compare Caller Publication Against Callee Consumption

Goal: determine whether the remaining abort comes from caller-side publication
or callee-side object/ABI interpretation.

Primary target: caller `test` emission, callee `dummy` emission, and prepared
object/ABI facts.

Actions:

- Compare the caller argument object contents against the prepared
  `call_argument_value_publications` payload facts.
- Compare the callee parameter object and `va_arg` load against the RV64
  `va_list` layout used by the caller.
- Identify whether the passed value should be the save-area pointer payload, a
  pointer to a copied `va_list` object, or another ABI-owned representation.
- Check clang disassembly or runtime behavior only to classify semantics, not
  to copy instruction shape.
- Record the owner classification in `todo.md`.

Completion check: `todo.md` classifies the boundary as caller publication,
callee consumption/object-ABI, or later split, with concrete evidence.

### Step 3: Route Or Repair The Owned Boundary

Goal: either repair the remaining idea-392 boundary or create a precise split
proposal for a later owner.

Primary target: the narrow code/test surface identified in Step 2.

Actions:

- If caller publication is still wrong, repair the explicit prepared
  publication path and add focused coverage for the post-repair divergence.
- If callee consumption is wrong but required to make `va_list` expression
  call arguments work, repair the narrow object/ABI state in this plan and add
  focused coverage.
- If the caller/callee handoff is correct and the abort is a distinct later
  feature, stop implementation and record the split recommendation in
  `todo.md`; do not silently expand this plan.
- Preserve existing backend proof and fail-closed publication tests.

Completion check: focused coverage or split evidence matches the Step 2 owner
classification without testcase-shaped matching.

### Step 4: Reprove And Decide Disposition

Goal: prove idea 392 completion or provide a canonical later-boundary handoff.

Primary target: focused backend tests and `va-arg-13.c`.

Actions:

- Run the supervisor-delegated backend proof command.
- Rerun the `va-arg-13.c` RV64 object-route representative.
- If the representative advances past the abort or reaches a later clearly
  owned boundary, record exact evidence in `todo.md`.
- If a separate initiative is required, ask the plan owner to split it instead
  of continuing implementation inside this runbook.
- Keep `test_after.log` as the canonical executor proof log unless the
  supervisor delegates another artifact.

Completion check: `todo.md` contains proof logs and a lifecycle-ready
disposition: completion evidence for idea 392, or exact split/next-owner
evidence.
