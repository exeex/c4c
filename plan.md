# RV64 Object Route Instruction Fragment Lowering Runbook

Status: Active
Source Idea: ideas/open/377_rv64_object_route_instruction_fragment_lowering.md

## Purpose

Repair the next RV64 object-route prepared instruction-fragment blocker exposed
after prepared move-bundle target-shape support for the `src/20000217-1.c`
representative.

## Goal

Identify the first unsupported ordinary prepared instruction-fragment shape,
admit one semantic RV64 object lowering slice when supportable, and prove that
the representative either passes or advances to a distinct next owner.

## Core Rule

Use prepared instruction, type, operand, and home metadata as the source of
truth. Do not key behavior on testcase names, source syntax, value ids, frame
slot numbers, instruction indexes, or prepared-BIR text matching.

## Read First

- Source idea: `ideas/open/377_rv64_object_route_instruction_fragment_lowering.md`
- Prior artifacts:
  - `build/agent_state/376_step5_20000217-1.runner.log`
  - `build/rv64_gcc_c_torture_backend/src_20000217-1.c/case.log`
  - `build/agent_state/376_step5_20000217-1.prepared-bir.txt`
  - `build/agent_state/376_step5_20000217-1.codegen-obj.log`
- Likely implementation surface:
  - `src/backend/mir/riscv/codegen/object_emission.cpp`
  - `tests/backend/mir/backend_riscv_object_emission_test.cpp`

## Current Scope

- Audit the first prepared instruction-fragment rejected by the RV64 object
  route after idea 376.
- Improve diagnostics only as needed to identify the semantic shape being
  rejected.
- Implement focused RV64 object emission for the first supportable ordinary
  prepared instruction shape.
- Add focused backend tests for the admitted shape and adjacent rejected
  shapes.
- Rerun the single `src/20000217-1.c` RV64 gcc-torture backend object
  representative and record the next result.

## Non-Goals

- Do not reopen scalar compare/trunc lowering from idea 375.
- Do not reopen prepared move-bundle target-shape support from idea 376.
- Do not broaden into terminator lowering, CFG reconstruction, globals,
  strings, data sections, aggregate `va_arg`, byval aggregate homes,
  non-register parameter homes, or call-argument ownership.
- Do not implement unrelated local-memory, publication, or return-ABI behavior
  unless the first audited instruction-fragment shape directly requires it.
- Do not downgrade unsupported tests, weaken contracts, or claim diagnostic-only
  work as backend capability progress.

## Working Model

The representative has advanced past:

```text
unsupported_move_bundle_target_shape: prepared move bundle requires unsupported RV64 moves
```

It now stops at:

```text
unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering
```

The first executor packet should establish the precise prepared instruction
shape behind that generic diagnostic before any lowering is admitted.

## Execution Rules

- Keep each code slice narrow and semantic.
- Preserve fail-closed behavior for adjacent unsupported shapes with explicit
  diagnostics.
- Prefer object-emission unit coverage before rerunning the gcc-torture
  representative.
- Update `todo.md` after each executor packet with the current step, evidence,
  proof command, result, and any next owner.
- If the representative advances to a distinct out-of-scope owner, stop and ask
  for lifecycle closure or handoff instead of silently expanding this idea.

## Ordered Steps

### Step 1: Audit First Instruction Fragment

Goal: identify the earliest ordinary prepared instruction-fragment shape that
the RV64 object route rejects after idea 376.

Primary target:

- `src/20000217-1.c` via the existing RV64 gcc-torture backend object route.

Actions:

- Regenerate or inspect the prepared/object-route evidence for the
  representative.
- Locate the first `unsupported_instruction_fragment` owner in prepared
  instruction terms.
- Document the instruction opcode, result type, operand types, operand homes,
  destination home, and any required stack/register publication behavior.
- If the generic diagnostic blocks precise ownership, make only the minimum
  diagnostic improvement needed for the audit.

Completion check:

- `todo.md` names the first rejected semantic instruction shape and records the
  evidence artifacts under `build/agent_state/` or the case log path.

### Step 2: Implement Focused Instruction Lowering

Goal: admit the first supportable ordinary prepared instruction-fragment shape
without broadening into unrelated object-route ownership.

Primary target:

- `src/backend/mir/riscv/codegen/object_emission.cpp`

Actions:

- Lower the audited shape using prepared instruction metadata, type metadata,
  operand homes, and destination home information.
- Keep adjacent unsupported operand/home/type combinations fail-closed with
  precise diagnostics.
- Avoid named-case, value-id, source-expression, or instruction-index matching.

Completion check:

- Focused backend object-emission tests prove the admitted shape and nearby
  rejected shapes.
- The implementation does not claim progress through expectation rewrites or
  unsupported-test downgrades.

### Step 3: Prove Focused Backend Coverage

Goal: verify the implementation slice against focused backend and prepared
contract coverage before representative rerun.

Primary targets:

- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- Existing prepared contract tests selected by the supervisor.

Actions:

- Run the supervisor-delegated build and focused CTest subset.
- Write executor proof to `test_after.log` unless the supervisor delegates a
  different artifact path.
- Record the exact command and result in `todo.md`.

Completion check:

- The focused backend proof is green, or `todo.md` records the exact blocker
  and no lifecycle closure is requested.

### Step 4: Rerun Representative

Goal: determine whether semantic instruction-fragment repair lets
`src/20000217-1.c` pass or advance to a distinct next owner.

Primary target:

- Single-case RV64 gcc-torture backend object representative for
  `src/20000217-1.c`.

Actions:

- Rerun the representative with an allowlist and stop-on-failure behavior.
- Confirm the prior instruction-fragment shape no longer fails.
- If a new failure appears, classify whether it remains in-scope for this idea
  or is a distinct next owner.

Completion check:

- `todo.md` records the representative result and supporting log paths.
- If the result is out-of-scope, `todo.md` includes enough detail for the plan
  owner to close this idea and create the next durable follow-up.

### Step 5: Closure Decision

Goal: decide whether the source idea is complete under its acceptance criteria.

Actions:

- Verify the first unsupported instruction-fragment shape was documented.
- Verify admitted lowering has focused backend tests and fail-closed adjacent
  coverage.
- Verify existing focused backend object-emission and prepared-contract
  coverage remains green.
- Verify the representative either passes or advances to a documented distinct
  next owner.

Completion check:

- The supervisor can delegate lifecycle closure, handoff, or route repair with
  no ambiguity about remaining ownership.
