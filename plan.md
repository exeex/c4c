# RV64 pr56982 Post-Carrier Runtime Mismatch Runbook

Status: Active
Source Idea: ideas/open/576_rv64_pr56982_post_carrier_runtime_mismatch.md

## Purpose

Diagnose and repair the RV64 object-route behavior that lets
`src/pr56982.c` compile past inline asm carrier lowering but then reach
`RV64_BACKEND_RUNTIME_MISMATCH` with a c4c segmentation fault.

## Goal

Identify the first post-carrier semantic owner for the runtime mismatch, add
focused proof for that owner, and repair it without changing inline asm carrier
diagnostics or route classification policy.

## Core Rule

Do not treat a changed runtime symptom, expectation rewrite, unsupported marker,
allowlist edit, or `src/pr56982.c` filename-specific path as progress.

## Read First

- `ideas/open/576_rv64_pr56982_post_carrier_runtime_mismatch.md`
- `build/agent_state/571_rv64_inline_asm_carrier_lowering/classification.tsv`
- `build/agent_state/571_rv64_inline_asm_carrier_lowering/summary.md`
- `build/agent_state/571_rv64_inline_asm_carrier_lowering/src_pr56982.c/object-route.log`
- `build/agent_state/571_rv64_inline_asm_carrier_lowering/src_pr56982.c/dump-prepared-bir.txt`

## Current Scope

- RV64 object-route behavior for `src/pr56982.c` after inline asm carrier
  lowering no longer blocks compilation.
- BIR, prepared-BIR, MIR, object emission, or runtime evidence needed to name
  the first semantic owner of the segfault or mismatch.
- Focused backend or route-level proof for the identified owner.

## Non-Goals

- Rework generic inline asm carrier lowering from idea 571.
- Broaden into call ABI, pointer arithmetic, select, floating-point binary, or
  phi-select lowering unless evidence proves that area is the first owner.
- Change runtime comparison files, expected outputs, unsupported markers,
  allowlists, or route accounting.
- Add testcase-shaped handling for `src/pr56982.c`.

## Working Model

The old carrier diagnostic is no longer the blocker. Treat the route as a
post-carrier runtime investigation: first preserve the failing route evidence,
then walk backward from the runtime mismatch to the earliest semantic owner
visible in prepared-BIR, MIR, object emission, or generated execution behavior.

## Execution Rules

- Keep evidence under `build/agent_state/576_rv64_pr56982_post_carrier_runtime_mismatch/`.
- Prefer focused tests near the owning backend layer once the owner is known.
- If the first owner is a separate durable initiative, record that fact for the
  supervisor instead of silently expanding this runbook.
- Update `todo.md` after each executor packet with the route result, owner
  evidence, and proof command.
- Use `test_after.log` for code-changing proof unless the supervisor delegates
  a different non-regression artifact.

## Steps

### Step 1: Reproduce post-carrier runtime mismatch and collect evidence

Goal: Preserve a fresh route snapshot showing that `src/pr56982.c` is past the
inline asm carrier diagnostic and now fails at runtime.

Primary target:
- `src/pr56982.c` RV64 object route

Actions:
- Rerun the representative RV64 object route for `src/pr56982.c`.
- Save the command, return code, stdout, stderr, merged route log, and any
  prepared-BIR dump under this idea's `build/agent_state/` directory.
- Confirm the route no longer reports the old inline asm carrier compile
  diagnostic as the first failure.

Completion check:
- `todo.md` records the exact route command, artifact paths, exit code, and
  current mismatch or segfault symptom.

### Step 2: Identify the first post-carrier semantic owner

Goal: Name the earliest semantic owner that explains the runtime mismatch or
c4c segfault.

Primary target:
- BIR, prepared-BIR, MIR, object emission, or runtime evidence from Step 1

Actions:
- Inspect the saved dumps and route log from Step 1.
- Trace the failing value, control path, call, memory access, or generated
  instruction sequence backward to the earliest unsupported or mis-lowered
  owner.
- Classify whether the owner is local to this idea or should become a
  separate source idea before implementation.

Completion check:
- `todo.md` records the owner, evidence paths, relevant function/value or
  instruction identifiers, and the recommended implementation surface.

### Step 3: Add focused proof for the identified owner

Goal: Create a narrow proof that fails for the owner for the right reason
before changing lowering behavior.

Primary target:
- The smallest backend or route-level test surface that covers the Step 2
  owner

Actions:
- Add or adjust focused coverage that exercises the semantic owner without
  matching `src/pr56982.c` by name.
- Keep any expected failure diagnostic or runtime assertion tied to semantic
  facts, not testcase shape.
- Run the supervisor-delegated focused proof command.

Completion check:
- `test_after.log` or the delegated proof artifact shows the focused proof
  reaches the intended owner before the repair.

### Step 4: Repair the owner without changing carrier or route policy

Goal: Implement the smallest semantic repair that fixes the Step 2 owner.

Primary target:
- The backend layer identified in Step 2

Actions:
- Modify only the owning lowering, object emission, runtime, or helper logic
  needed for the identified owner.
- Preserve inline asm carrier diagnostics and unsupported classification.
- Avoid filename-specific matching, expectation rewrites, allowlist changes,
  and broad unrelated backend rewrites.
- Run build plus the focused proof command delegated by the supervisor.

Completion check:
- Focused proof passes and `todo.md` records the changed files, command, and
  result.

### Step 5: Re-run the representative route and backend validation

Goal: Prove `src/pr56982.c` no longer fails for the same post-carrier reason
and the backend bucket remains stable.

Primary target:
- `src/pr56982.c` RV64 object route
- Backend regression subset

Actions:
- Rerun the representative route and save the same artifact set used in
  Step 1.
- If a later distinct owner remains, record it clearly for supervisor
  lifecycle handling.
- Run the supervisor-selected backend validation subset.

Completion check:
- The old `RV64_BACKEND_RUNTIME_MISMATCH` with c4c segfault for the same reason
  is gone, or the remaining failure is documented as a later distinct owner.
- Backend validation passes or the precise blocker is recorded in `todo.md`.
