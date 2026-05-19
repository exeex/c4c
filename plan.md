# AArch64 Indirect Call Argument Preservation Runbook

Status: Active
Source Idea: ideas/open/309_aarch64_indirect_call_argument_preservation.md
Activated from: idea 310 completion after indirect-call string argument facts were repaired

## Purpose

Repair the focused `00189.c` runtime segfault as a semantic AArch64
indirect-call callee and argument-preservation bug, not as a broad
call-lowering bucket.

## Goal

Make the focused indirect function-pointer call preserve its callee and outer
arguments across nested call setup.

## Core Rule

Progress must come from semantic call-lowering repair. Do not improve counts by
matching `00189.c`, rewriting expectations, allowlists, unsupported
classifications, CTest registration, runner behavior, timeout policy, proof
logs, or test contracts.

## Read First

- `ideas/open/309_aarch64_indirect_call_argument_preservation.md`
- `ideas/closed/310_prepared_indirect_call_string_argument_facts.md`,
  especially the completion note proving `.str1` prepared facts now publish
- `tests/c/external/c-testsuite/src/00189.c`
- `build/c_testsuite_aarch64_backend/src/00189.c.s`
- Closed owner 308 only as the boundary proving the old extern-data PIC
  relocation failure is already repaired

## Current Scope

- Focused c-testsuite target `c_testsuite_aarch64_backend_src_00189_c`
- AArch64 indirect function-pointer call setup in `00189.c`
- Preservation of the indirect callee and outer-call arguments across nested
  direct or indirect call setup
- Re-checking the known `fprintfptr` callee register placement mismatch now
  that the `.str1` prepared string argument fact is available
- Narrow backend tests that can prove semantic indirect-call preservation if
  the implementation needs local coverage

## Non-Goals

- No direct multi-argument call shuffle owner for `00181.c` or `00182.c`.
- No direct vararg aliasing/materialization owner for `00200.c`.
- No address-of-local direct-call argument preparation owner for `00218.c`.
- No timeout, output-storm, dynamic-stack/goto, string/pointer/store/control,
  or unrelated runtime mismatch repairs.
- No reopening closed owners 285 through 308 without new contradictory
  generated-code or proof evidence.
- No expectations, allowlists, unsupported classifications, runner policy,
  timeout policy, CTest registration, proof-log, or test-contract edits.

## Working Model

- Idea 308 removed the old `00189.c` link failure by routing externally
  binding `stdout` through GOT materialization.
- Idea 310 repaired the prerequisite producer fact: focused prepared output for
  `00189.c` now publishes
  `address_materialization block=entry inst_index=4 kind=string_constant
  result=@.str1 text=.str1 ...`.
- The residual failure remains in AArch64 call-boundary lowering, where the
  outer indirect callee and fixed arguments must survive setup for the nested
  `(*f)(24)` result that becomes a later vararg.
- The last observed AArch64 hazard was that prepared storage printed `%t0` as
  `reg=x21`, while ordinary non-GOT `load_global` lowering loaded `fprintfptr`
  through the ABI pool as `x20`. Re-check that callee placement mismatch before
  choosing the final preservation repair.
- Neighbor classification parked direct-call bugs separately. Do not fold those
  cases into this owner without a supervisor-approved lifecycle split.

## Execution Rules

- Keep routine implementation progress in `todo.md`.
- Start from the current prepared facts instead of revisiting the missing
  `.str1` producer; that prerequisite is now repaired.
- Make the next code slice explain which call-lowering surface owns callee and
  argument preservation; avoid broad rewrites until a narrow ownership point is
  identified.
- Add local tests only when they prove the semantic preservation rule and would
  fail for the current bad lowering.
- Preserve proof artifacts and lifecycle boundaries unless the supervisor
  explicitly delegates a different proof/log policy.

## Ordered Steps

### Step 1: Locate the Indirect-Call Preservation Owner

Goal: identify the backend lowering surface responsible for preserving the
indirect callee and outer-call arguments across nested call setup.

Primary targets: `tests/c/external/c-testsuite/src/00189.c`, generated
`00189.c.s`, and the AArch64 call-lowering implementation surface.

Actions:

- Confirm the current crash shape still matches clobbered indirect callee or
  outer-call argument setup, not the closed extern-data PIC relocation owner.
- Trace how the indirect callee value and each outer-call argument are assigned
  physical call registers.
- Identify whether the lowering needs a parallel-copy, temporary preservation,
  call-sequence ordering, or value-liveness repair.
- Do not edit expectations, allowlists, runner policy, CTest registration, or
  proof logs.

Completion check:

- `todo.md` records the owned lowering surface, the failing value-preservation
  path, and the narrow proof command the supervisor delegated.

### Step 2: Repair Callee and Argument Preservation

Goal: implement the smallest semantic repair that preserves the indirect
callee and outer-call arguments until the final `blr`.

Actions:

- Re-check the generated `00189.c` prepared facts and AArch64 assembly with the
  repaired `.str1` materialization available.
- Inspect the remaining `fprintfptr` callee register placement mismatch before
  choosing the repair surface.
- Verify whether the nested call result and outer-call argument homes overlap
  saved-register storage or stale prepared homes.
- Repair the chosen call-lowering surface without matching `00189.c` by name or
  source shape.
- Preserve existing direct-call behavior unless the same semantic helper
  naturally owns it and the supervisor approves the scope.
- Add or adjust focused backend tests when needed to lock indirect callee and
  nested-call argument preservation.

Completion check:

- A fresh build or compile proof succeeds for the changed code.
- The generated focused assembly no longer clobbers the indirect callee or
  required outer-call arguments before `blr`.

### Step 3: Prove the Focused Runtime Target

Goal: prove the semantic repair against the focused c-testsuite target and
adjacent call-boundary coverage selected by the supervisor.

Actions:

- Run the delegated focused c-testsuite proof for
  `c_testsuite_aarch64_backend_src_00189_c`.
- Run any delegated local backend/unit proof that covers indirect-call
  preservation.
- Record results in `todo.md` and canonical proof logs as directed by the
  supervisor.

Completion check:

- `00189.c` no longer fails from the old indirect-call callee/argument
  clobbering mode, or `todo.md` records the new residual boundary if a
  different runtime failure remains.

### Step 4: Completion Review

Goal: decide whether the focused owner is complete or should return residuals
to umbrella idea 295.

Actions:

- Compare the final behavior against this source idea's acceptance criteria and
  reviewer reject signals.
- If complete, ask the plan owner to close the focused idea after required
  regression guard proof.
- If residuals remain outside this owner, return only those residuals to idea
  295 with a durable note.

Completion check:

- Lifecycle state is ready for close, or `todo.md` names the remaining blocker
  and the correct umbrella bucket.
