# RV64 Object Route Short-Circuit Select Join Materialization Runbook

Status: Active
Source Idea: ideas/open/381_rv64_object_route_short_circuit_select_join_materialization.md

## Purpose

Continue `src/20000112-1.c` after idea 380 repaired the call-argument
prior-preservation owner and the representative advanced to a distinct
short-circuit select/join materialization failure.

Goal: repair the RV64 object route so skip-edge and join materialization uses
the prepared authoritative select/join value instead of reading an RHS result
home on paths where the RHS did not run.

## Core Rule

Use prepared BIR, phi-edge move bundles, value-home, publication, and
object-route facts. Do not key behavior on `src/20000112-1.c`,
`special_format`, `strchr`, exact C expression spelling, block labels, value
ids, instruction indexes, physical registers, object addresses, or log text.

## Read First

- `ideas/open/381_rv64_object_route_short_circuit_select_join_materialization.md`
- `todo.md`
- `build/agent_state/380_residual_followup_20000112.classification.txt`
- `build/agent_state/380_residual_followup_20000112.special_format_disasm.txt`
- `build/agent_state/380_residual_followup_20000112.prepared_bir.txt`
- `build/rv64_gcc_c_torture_backend/src_20000112-1.c/case.log`

## Current Targets

- Primary representative: `src/20000112-1.c`
- Failure class entering this pass: `RV64_BACKEND_RUNTIME_MISMATCH`
- Current symptom: object code reaches a short-circuit join and reads `s2` on
  skip edges where the RHS call result was never produced.
- Current owner: RV64 object-route short-circuit select/join materialization,
  not the idea 380 call-argument prior-preservation owner.

## Non-Goals

- Do not reopen idea 380 unless focused coverage shows call-argument
  prior-preservation regressed.
- Do not reopen idea 379 unless focused coverage shows its earlier
  select/publication repair regressed.
- Do not add testcase-name, `special_format`, `strchr`, block-label,
  value-id, register, address, or log-text handling.
- Do not broaden the route into full CFG reconstruction, register allocation,
  ABI redesign, assembler/object replacement, byval aggregate homes, aggregate
  `va_arg`, globals, or strings unless the audit proves the current prepared
  edge/value-home facts cannot express the repair.
- Do not weaken runtime expectations, external allowlists, supported-path
  contracts, or focused backend coverage.

## Working Model

- Prepared BIR already contains phi-edge immediate materialization bundles for
  short-circuit skip edges.
- RV64 object emission must publish or preserve the authoritative select/join
  value on every incoming edge before join consumers read it.
- The first packet should verify whether the missing lowering is edge move
  emission, join publication placement, consumer source selection, or rejection
  of an unsupported shape.
- If the representative advances to a truly distinct owner after this repair,
  document that owner in `todo.md` instead of expanding this plan.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Keep implementation packets small and semantic.
- Preserve fail-closed behavior for unsupported or ambiguous select/join
  materialization shapes.
- For each code-changing step, run the focused backend build/test proof chosen
  by the supervisor.
- Keep ideas 379 and 380 focused coverage green.
- Rerun `src/20000112-1.c` after the admitted repair.

## Steps

### Step 1: Audit Select/Join Materialization Gap

Goal: identify why object emission reads the RHS result home on skip-edge join
paths instead of the prepared authoritative select/join value.

Actions:
- Inspect the starting evidence and current object-route emission around the
  first logic-end join that reads `s2`.
- Compare prepared BIR phi-edge move bundles with emitted object code for the
  corresponding predecessor edges and join consumers.
- Determine whether the missing rule is edge move emission, block-entry
  publication, join publication placement, consumer source selection, or an
  unsupported shape that must stay rejected.
- Record the smallest semantic shape that must be admitted and nearby shapes
  that must remain fail-closed.

Completion check:
- `todo.md` names the select/join materialization shape, the missing lowering
  rule, and the artifacts or commands used to identify it.

### Step 2: Implement Select/Join Materialization Repair

Goal: emit or select the correct prepared select/join value for skip-edge joins
without testcase-shaped matching.

Actions:
- Implement the narrow semantic repair in the responsible RV64 object-route
  lowering surface.
- Base the repair on prepared BIR, phi-edge move bundles, value-home, and
  publication facts.
- Ensure skip edges that bypass RHS calls do not read unproduced RHS result
  homes.
- Keep idea 379 and idea 380 repairs intact.

Completion check:
- Focused backend tests prove the admitted short-circuit skip-edge/join
  materialization behavior and reject adjacent unsupported or ambiguous shapes.
- Existing focused backend coverage for ideas 379 and 380 remains green.
- The supervisor-delegated focused build and test command passes.

### Step 3: Rerun `src/20000112-1.c`

Goal: confirm the representative result after the select/join repair.

Actions:
- Rerun `src/20000112-1.c` through the RV64 GCC torture backend progress path
  using the supervisor-provided command.
- Inspect any remaining failure to decide whether it is still the same
  select/join owner or a distinct next owner.
- If it is the same owner, record the remaining materialization gap in
  `todo.md` for another bounded executor packet.

Completion check:
- The representative passes, or `todo.md` documents a distinct next owner with
  artifacts and a handoff recommendation, or `todo.md` documents the still
  in-scope select/join gap that needs another plan-owner review.

### Step 4: Closure or Handoff Check

Goal: decide whether idea 381 is complete after the select/join repair.

Actions:
- Verify the source idea's select/join materialization acceptance criteria are
  satisfied.
- Confirm focused backend coverage for the admitted select/join behavior still
  passes.
- Confirm ideas 379 and 380 focused coverage still passes.
- Ask the plan owner to close idea 381 only if the source idea acceptance
  criteria are satisfied and regression guard can pass.

Completion check:
- Either the supervisor has enough evidence to request closure of idea 381, or
  `todo.md` clearly identifies the remaining blocker and why it is still in
  scope.
