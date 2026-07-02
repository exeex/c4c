# RV64 Object Select And Edge Publication Helper Cleanup Runbook

Status: Active
Source Idea: ideas/open/540_rv64_object_select_edge_publication_helper_cleanup.md

## Purpose

Extract RV64 object-route select-source, publication-move, and predecessor-edge helper fragments after scalar and local/global helper APIs are stable.

## Goal

Make select and edge-publication helper ownership explicit without changing prepared publication facts, fallback behavior, diagnostics, object bytes, or testcase contracts.

## Core Rule

This runbook is behavior-preserving cleanup. Do not repair RV64 capability, rewrite expectations, weaken unsupported markers, or hide `fragment_for_prepared_instruction` fanout behind another broad dispatcher.

## Read First

- `ideas/open/540_rv64_object_select_edge_publication_helper_cleanup.md`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/prepared_edge_publication_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_edge_publication_emit.hpp`
- `src/backend/mir/riscv/codegen/prepared_scalar_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_scalar_emit.hpp`

## Current Targets

- Select-source producer classification and carrier-authority helper fragments in `object_emission.cpp`.
- Select publication move admission, diagnostics, and predecessor-edge fragment helpers in `object_emission.cpp`.
- Existing prepared edge publication APIs in `prepared_edge_publication_emit.cpp` / `.hpp` where ownership already belongs there or can be narrowed without hiding dependencies.
- Existing prepared scalar APIs only when the select/publication helper needs scalar materialization through an explicit dependency.

## Non-Goals

- Do not move generic scalar movement, local/global memory movement, final object module assembly, or broad `fragment_for_prepared_instruction` dispatch.
- Do not change prepared publication planning, publication facts, fallback behavior, branch behavior, diagnostics, object bytes, test expectations, unsupported markers, or runtime contracts.
- Do not introduce testcase-shaped select fixes or named-case shortcuts.
- Do not make select/publication cleanup depend on target-side inference that bypasses prepared facts.

## Working Model

- `prepared_edge_publication_emit.*` may own select publication movement and predecessor-edge helper logic when its API keeps publication plans, edge metadata, scalar materialization, and consumer classification dependencies visible.
- `object_emission.cpp` should retain top-level object-route orchestration and dispatch, but should stop owning narrow select/publication helper bodies once the dependencies are explicit.
- `prepared_scalar_emit.*` remains scalar-focused. Use it as an explicit dependency only for scalar materialization helpers already owned there; do not turn it into select/publication ownership.

## Execution Rules

- Keep each step small enough for one executor packet and one proof command.
- Prefer extracting cohesive helper groups over renaming isolated functions.
- Preserve diagnostic strings and rejection reasons unless the supervisor explicitly approves a behavior change.
- After each code-changing step, run the narrow proof command selected by the supervisor and write the result to `test_after.log`.
- Escalate to broader validation if a step touches prepared publication planning, fallback behavior, branch selection, object bytes, or shared dispatch.

## Steps

### Step 1: Map Select And Publication Ownership

Goal: identify which select/publication helpers are already edge-publication-owned, which remain object-only, and which must stay object-side orchestration.

Primary target: `src/backend/mir/riscv/codegen/object_emission.cpp`

Actions:

- Inspect select-source producer, select consumer classification, publication move, predecessor-edge, and rejection-diagnostic helpers.
- Record helper groups and proposed ownership in `todo.md`; keep this as execution state, not source-idea churn.
- Identify any helper that must remain in `object_emission.cpp` because it depends on broad dispatch, object function traversal, or non-select publication behavior.
- Identify compile boundaries and header declarations needed before moving code.

Completion check:

- `todo.md` names the first extraction target and documents why each deferred helper is out of the first packet.
- No implementation files are changed in this step unless the supervisor explicitly delegates a code packet.

### Step 2: Extract Narrow Select Publication Move Helpers

Goal: move narrow select publication move admission, agreement, and diagnostic helper logic behind explicit prepared edge publication APIs.

Primary targets:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/prepared_edge_publication_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_edge_publication_emit.hpp`

Actions:

- Move only helper bodies whose inputs can be expressed as prepared publication, bundle, edge, names, and scalar/materialization dependencies.
- Keep fallback behavior, rejection reason strings, and prepared publication fact checks unchanged.
- Publish declarations only for helpers that object orchestration still needs to call.
- Leave broad dispatch and function traversal in `object_emission.cpp`.

Completion check:

- The moved helper group builds through the normal target.
- The supervisor-selected select/publication CTest subset passes.
- The diff does not alter tests, expectations, unsupported markers, or diagnostic contracts.

### Step 3: Extract Predecessor Edge Fragment Helpers

Goal: move predecessor-edge select publication fragment construction that is already driven by prepared edge publication facts.

Primary targets:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/prepared_edge_publication_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_edge_publication_emit.hpp`

Actions:

- Extract the GPR-to-stack and pointer-stack-source-to-GPR predecessor-edge fragment helpers if their dependencies remain explicit.
- Preserve bundle admission checks, stack-home checks, scalar materialization paths, and route5/route3 agreement validation.
- Keep object-side control-flow traversal responsible for choosing where predecessor-edge fragments are emitted.

Completion check:

- The extracted fragment helpers compile and remain called from object orchestration.
- The supervisor-selected select/publication CTest subset passes.
- Diagnostic and fallback behavior are byte-for-byte or semantically unchanged.

### Step 4: Narrow Select Source Producer Dependencies

Goal: reduce object-side coupling around selected source producer placement and select consumer dependencies without moving broad instruction dispatch.

Primary targets:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/prepared_edge_publication_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_edge_publication_emit.hpp`
- `src/backend/mir/riscv/codegen/prepared_scalar_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_scalar_emit.hpp`

Actions:

- Inspect select-source producer helpers that depend on scalar materialization or prepared consumer classification.
- Move only helpers whose ownership is naturally edge-publication or scalar, and expose required context through narrow parameters.
- Leave any helper that depends on `fragment_for_prepared_instruction` fanout or whole-function traversal in `object_emission.cpp`.

Completion check:

- Select-source helper ownership is narrower and documented in `todo.md`.
- The supervisor-selected select/publication CTest subset passes.
- No capability repair, expectation rewrite, unsupported marker change, or diagnostic weakening is present.

### Step 5: Final Review And Close Readiness

Goal: confirm the cleanup achieved narrower select/publication ownership and is ready for lifecycle closure evaluation.

Actions:

- Review the final diff against the source idea's in-scope and out-of-scope sections.
- Confirm prepared publication facts, fallback behavior, diagnostics, branch behavior, object bytes, and test contracts were not intentionally changed.
- Run or request the full source-idea validation command:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(riscv_prepared_edge_publication|dump_riscv64_.*select|codegen_route_riscv64_.*select|rv64_runtime_riscv64_pointer_typed_select_publication|rv64_runtime_riscv64_short_circuit_select_false_lhs)'`

Completion check:

- `todo.md` records proof and any residual deferred helpers.
- The source idea is ready for plan-owner closure evaluation, or `todo.md` explains why the runbook is exhausted but the source idea remains open.
