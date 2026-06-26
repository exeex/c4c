# RV64 Object Route 20000112 Runtime Join Publication Runbook

Status: Active
Source Idea: ideas/open/379_rv64_object_route_20000112_runtime_join_publication.md

## Purpose

Repair the RV64 object-route runtime mismatch now exposed by `src/20000112-1.c`
after the prior terminator and same-width `ZExt` compile blockers were fixed.

Goal: prove and repair the first semantic value-publication failure for
`src/20000112-1.c`, then rerun the representative to confirm it either passes
or advances to a distinct next owner.

## Core Rule

Use prepared CFG, value-home, publication, and object-route facts. Do not key
behavior on testcase name, C source spelling, exact value ids, physical
register names, block labels, instruction indexes, objdump addresses, or log
text.

## Read First

- `ideas/open/379_rv64_object_route_20000112_runtime_join_publication.md`
- `build/rv64_gcc_c_torture_backend/src_20000112-1.c/case.log`
- `build/agent_state/378_step5_20000112.c4c_objdump.log`
- `build/agent_state/378_step5_20000112.qemu_strace.err`
- `build/agent_state/378_step5_20000112.qemu_strace.out`
- `test_before.log`
- `test_after.log`

## Current Target

- Primary representative: `src/20000112-1.c`
- Failure class entering this plan: `RV64_BACKEND_RUNTIME_MISMATCH`
- Reported symptom: `clang_exit=0 c4c_exit=Segmentation fault`
- Suspected owner from handoff evidence: join/select publication in
  `special_format`, where skip blocks materialize a short-circuit value in one
  GPR while later join blocks read a different GPR before comparing.

## Non-Goals

- Do not reopen idea 369 terminator-fragment lowering.
- Do not reopen idea 378 same-width `ZExt` instruction-fragment lowering.
- Do not repair globals, strings, byval aggregate homes, aggregate `va_arg`,
  unrelated non-register parameter homes, or unrelated call-argument behavior.
- Do not introduce broad CFG reconstruction, register-allocation replacement,
  or assembler/object-route replacement unless the audit proves the prepared
  publication contract cannot express the semantic repair.
- Do not downgrade unsupported expectations or weaken runtime contracts.

## Working Model

- Treat this as a runtime value-publication contract problem until evidence
  proves otherwise.
- First identify the semantic publication edge or join shape from prepared and
  emitted object evidence.
- Admit only the first supportable shape with focused fail-closed coverage for
  adjacent ambiguous shapes.
- If the representative advances to a different owner, document that owner in
  `todo.md` rather than expanding this plan.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Keep implementation packets small and semantic.
- For each code-changing step, run the focused backend build/test proof chosen
  by the supervisor.
- Rerun the representative after an admitted repair.
- Escalate to review or plan-owner repair if the route requires broad CFG or
  register-allocation redesign.

## Steps

### Step 1: Audit First Runtime Publication Mismatch

Goal: identify the first incorrect runtime value-publication shape for
`src/20000112-1.c`.

Actions:
- Rerun or inspect the representative runtime failure with prepared CFG,
  value-home, publication, and object-route artifacts.
- Determine whether the first wrong value is caused by block-entry
  publication, edge publication, select/short-circuit publication, or join
  ownership.
- Record the semantic shape, required operands, involved value homes, and why
  the current object emission reads the wrong value.

Completion check:
- `todo.md` names the first semantic publication shape and includes the proof
  artifacts or commands used to identify it.

### Step 2: Implement the First Semantic Publication Repair

Goal: lower the audited publication shape on the RV64 object route without
testcase-shaped matching.

Actions:
- Implement the narrow semantic repair in the responsible prepared/object-route
  lowering surface.
- Base the repair on prepared semantic facts and value-home/publication facts,
  not physical-register coincidences or source testcase details.
- Preserve existing fail-closed behavior for unsupported publication shapes.

Completion check:
- Focused backend tests prove the accepted shape and reject adjacent unsupported
  or ambiguous publication shapes.
- The supervisor-delegated focused build and test command passes.

### Step 3: Rerun `src/20000112-1.c`

Goal: confirm the representative result after the publication repair.

Actions:
- Rerun `src/20000112-1.c` through the RV64 GCC torture backend progress script
  using the supervisor-provided allowlist command.
- Inspect any remaining failure to decide whether it is still the same
  publication owner or a distinct next owner.

Completion check:
- The representative passes, or `todo.md` documents a distinct next owner with
  artifacts and a handoff recommendation.

### Step 4: Closure or Handoff Check

Goal: decide whether idea 379 is complete or needs a follow-up lifecycle split.

Actions:
- Verify the first runtime publication mismatch from this source idea has been
  repaired or intentionally handed off as a distinct owner.
- Confirm focused backend/prepared coverage still passes.
- Ask the plan owner to close this idea only if the source idea acceptance
  criteria are satisfied and regression guard can pass.

Completion check:
- Either the supervisor has enough evidence to request closure of idea 379, or
  `todo.md` clearly identifies the remaining blocker and why it is still in
  scope.
