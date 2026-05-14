# AArch64 Scalar Return ALU Selected Nodes Runbook

Status: Active
Source Idea: ideas/open/222_aarch64_scalar_return_alu_selected_nodes.md

## Purpose

Implement the first markdown-selected AArch64 backend feature group: scalar integer add/sub values returned through the normal selected machine-node and printer path.

## Goal

Enable `tests/backend/case/return_add.c` only after `alu.md` and `returns.md` have structured record, selected-node/operator, machine-printer, and focused C++ test coverage for scalar add plus returned result emission.

## Core Rule

Do not make `return_add.c` pass through named-case matching, fixture text, or weakened expectations. Capability must flow through the markdown-owned route:

```text
src/backend/mir/aarch64/codegen/alu.md
  + src/backend/mir/aarch64/codegen/returns.md
  -> src/backend/mir/aarch64/codegen/records.cpp/.hpp
  -> selected scalar machine-node/operator coverage
  -> src/backend/mir/aarch64/codegen/machine_printer.cpp
  -> tests/backend/case/return_add.c
```

## Read First

- `ideas/open/222_aarch64_scalar_return_alu_selected_nodes.md`
- `src/backend/mir/aarch64/BACKEND_CASE_BRINGUP_MATRIX.md`
- `src/backend/mir/aarch64/MACHINE_INSTRUCTION_NODE_CONTRACT.md`
- `src/backend/mir/aarch64/codegen/alu.md`
- `src/backend/mir/aarch64/codegen/returns.md`
- `src/backend/mir/aarch64/codegen/records.cpp`
- `src/backend/mir/aarch64/codegen/records.hpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- existing `tests/backend/backend_aarch64_*` C++ tests
- `tests/backend/case/return_add.c`

## Current Targets And Scope

- Scalar integer add/sub records and return payload records needed for a returned add result.
- Natural selected operator names and printable machine-node/operator coverage.
- Semantic instruction selection for simple integer add/sub results returned through the current AArch64 backend route.
- Terminal `.s` machine-printer output for add/sub plus return, using structured selected nodes/operators.
- Public `return_add.c` smoke on `c4cll --codegen asm --target aarch64-linux-gnu` after the internal route is proven.

## Non-Goals

- Do not enable `return_add_sub_chain.c`, two-argument ALU fixture groups, pointer returns, builtin abs, comparison, memory, call, globals, aggregate, variadic, inline asm, FP/SIMD, atomic, intrinsic, i128, object, linker, or binary-output behavior.
- Do not replace the terminal `.s` printer route with the internal assembler, encoder, object writer, linker, or binary route.
- Do not weaken existing enabled AArch64 smoke expectations, mark supported paths unsupported, or loosen failure guards.
- Do not edit idea 221 artifacts except normal lifecycle history already completed.
- Do not touch `ideas/open/220_backend_test_tree_split_bir_mir_and_prune_legacy.md` from this plan.

## Working Model

This plan should build capability from internal contracts outward:

1. prove the markdown contracts and current code/test surfaces;
2. add structured records and focused unit coverage for scalar ALU and return payload behavior;
3. add selected machine-node/operator lowering for returned scalar add/sub results;
4. teach the machine printer to emit the selected add/sub and return sequence from structured nodes;
5. enable the public `return_add.c` smoke with external assembler/compiler proof.

Each code-changing step should leave the public case disabled until the structured route underneath it is covered.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Preserve source intent in the idea file; do not edit it for ordinary execution notes.
- Prefer semantic lowering and reusable selected-node/operator coverage over testcase-shaped matching.
- For code-changing steps, run `cmake --build --preset default` or the repo-equivalent build command before narrow proof.
- Use focused C++ tests for records, selected operators, natural mnemonic spelling, return payloads, and machine-printer behavior before enabling the public case.
- When a step touches public smoke wiring or backend case expectations, run the narrow backend case command and escalate to broader backend validation before close.
- Treat testcase overfit as a blocking failure even if `return_add.c` passes.

## Steps

### Step 1: Inspect Markdown Contracts And Existing AArch64 ALU/Return Surfaces

Goal: establish the exact existing surfaces that must change before any implementation packet edits code.

Primary targets:
- `src/backend/mir/aarch64/codegen/alu.md`
- `src/backend/mir/aarch64/codegen/returns.md`
- `src/backend/mir/aarch64/codegen/records.cpp`
- `src/backend/mir/aarch64/codegen/records.hpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- existing `tests/backend/backend_aarch64_*` tests
- `tests/backend/case/return_add.c`

Actions:
- Inspect the scalar ALU and return markdown contracts for the minimal add/sub plus returned-result obligations.
- Identify current record types, operator names, machine-node shapes, printer behavior, and focused tests already present.
- Identify the narrow C++ test targets and backend case command the supervisor should delegate for the next implementation packet.
- Confirm the public `return_add.c` smoke is not enabled ahead of structured selected-node/operator coverage.

Completion check:
- `todo.md` records the exact surfaces inspected, current gaps, and the recommended first implementation packet.
- Search or test proof records the files and test targets inspected.

### Step 2: Add Scalar ALU And Return Record Coverage

Goal: convert the relevant `alu.md` and `returns.md` contracts into structured record coverage.

Primary targets:
- `src/backend/mir/aarch64/codegen/records.cpp`
- `src/backend/mir/aarch64/codegen/records.hpp`
- focused `tests/backend/backend_aarch64_*` C++ tests

Actions:
- Add or strengthen records for scalar add/sub operators and returned scalar payload behavior.
- Preserve natural mnemonic spelling and existing operator naming conventions.
- Add focused tests that prove records, operator names, and return payload records independently of the public smoke.
- Avoid adding fixture-specific fields or named `return_add` shortcuts.

Completion check:
- Focused C++ tests prove scalar ALU records, return payload records, and natural operator names.
- Build proof is recorded in `test_after.log` or the supervisor-delegated proof log.

### Step 3: Select Returned Scalar Add/Sub Machine Nodes

Goal: lower simple returned integer add/sub results into selected AArch64 machine nodes/operators through the normal backend path.

Primary targets:
- AArch64 selection/lowering code that consumes prepared backend/MIR input
- selected machine-node/operator model
- focused `tests/backend/backend_aarch64_*` C++ tests

Actions:
- Add semantic selection for simple integer add/sub results that feed a return value.
- Reuse the structured record and operator coverage from Step 2.
- Add focused tests proving selected add/sub machine nodes and returned-result sequencing.
- Keep broader ALU forms deferred unless they naturally fall out of the same semantic route and receive matching proof.

Completion check:
- Focused tests prove selected add/sub and returned-result machine-node/operator behavior without relying on public smoke fixture matching.
- Build plus narrow selected-node test proof is recorded.

### Step 4: Print Structured Add/Sub And Return Assembly

Goal: emit the selected add/sub plus return sequence from structured machine nodes/operators.

Primary targets:
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- focused machine-printer tests

Actions:
- Teach the machine printer to spell selected add/sub operators and return sequence from structured nodes.
- Add or strengthen printer tests for natural mnemonic spelling, add/sub operands, and `ret`.
- Do not emit hard-coded fixture assembly for `return_add.c`.

Completion check:
- Focused printer tests prove emitted add/sub and return assembly shape from selected nodes/operators.
- Build plus narrow printer proof is recorded.

### Step 5: Enable Public `return_add.c` Smoke

Goal: enable or add the public backend case only after the structured route is covered.

Primary targets:
- `tests/backend/case/return_add.c`
- backend case smoke runner and expected assembly checks
- external assembler/compiler proof hooks

Actions:
- Enable or add the public smoke on `c4cll --codegen asm --target aarch64-linux-gnu`.
- Assert real AArch64 assembly shape including an `add` operation and `ret`.
- Preserve external assembler/compiler proof according to the existing host-gated smoke model.
- Keep `return_add_sub_chain.c` and broader scalar ALU cases deferred unless they have matching structured proof.

Completion check:
- Public `return_add.c` smoke passes through the normal backend path with structured add plus return output.
- External assembler/compiler proof is recorded where the existing smoke model requires it.

### Step 6: Final Validation And Close Readiness

Goal: verify idea 222 acceptance without drifting into broader AArch64 backend work.

Primary targets:
- focused record/selection/printer tests
- public `return_add.c` smoke
- `todo.md`

Actions:
- Re-run the focused C++ tests for records, selected operators, and machine-printer behavior.
- Re-run the public backend case smoke for `return_add.c`.
- Run broader backend validation if selection, printer, or smoke wiring changes touched shared backend behavior.
- Confirm no supported case was weakened, hidden, or marked unsupported.
- Confirm deferred scalar ALU and broader AArch64 cases remain documented as outside this idea unless they gained real structured proof.

Completion check:
- Source idea acceptance criteria are satisfied.
- Regression guard can be run by the plan owner before closure for the chosen close scope.
