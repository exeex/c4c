# RV64 Prepared Edge Publication Module Flow Runbook

Status: Active
Source Idea: ideas/open/328_rv64_prepared_edge_publication_module_flow.md

## Purpose

Repair RV64 prepared module/function emission so prepared block-entry
edge-publication moves from shared prepared facts are spliced into final
assembly when they are available.

## Goal

Make `backend_riscv_prepared_edge_publication` pass by emitting the required
RV64 register-edge publication move in final prepared module text.

## Core Rule

Consume shared prepared `edge_publications` authority through the existing
RV64 edge-publication helper path; do not rediscover moves by scanning BIR
predecessor/successor block shapes or by special-casing the focused fixture.

## Read First

- `ideas/open/328_rv64_prepared_edge_publication_module_flow.md`
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`
- `src/backend/mir/riscv/codegen/prepared_module_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_function_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_edge_publication_emit.cpp`
- `src/backend/mir/riscv/codegen/emit.hpp`

## Current Targets

- Focused test: `backend_riscv_prepared_edge_publication`
- Suspected orchestration site:
  `src/backend/mir/riscv/codegen/prepared_module_emit.cpp`
- Existing simple function path:
  `append_simple_prepared_bir_function_asm(...)`
- Existing helper path:
  `append_edge_publication_move_instruction(...)`

## Non-Goals

- Do not change RV64 `mv` spelling or treat register moves as unsupported.
- Do not rework AArch64 edge-copy/publication lowering.
- Do not migrate prepared edge-publication authority into BIR or target-local
  fact tables.
- Do not touch the separate idea 327 frontend LLVM-route function-pointer
  return regression.
- Do not weaken the focused test, downgrade support expectations, or accept a
  helper-only proof when final assembly still omits the move.
- Do not perform a broad RV64 control-flow rewrite.

## Working Model

The failing prepared fixture has available edge-publication move facts for a
join edge, and the RV64 helper can render `mv a1, a0`. The suspected gap is
that `emit_prepared_module_text(...)` lets a successful
`append_simple_prepared_bir_function_asm(...)` path skip the later
edge-publication append path, so available moves never reach final module
assembly for the minimal prepared function.

## Execution Rules

- Keep changes in RV64 prepared module/function emission unless direct evidence
  shows the smallest valid repair belongs in the RV64 helper call surface.
- Preserve fail-closed behavior for unsupported source/destination homes,
  mismatched publications, and missing shared lookup authority.
- Prove final emitted assembly contains the expected register-edge move; do not
  rely only on helper availability.
- Treat fixture-name, value-id, register-name, or literal-output matching as
  route drift.
- Use `test_after.log` for executor proof output unless the supervisor
  delegates a different artifact.

## Ordered Steps

### Step 1: Reproduce And Localize The Missing Publication Move

Goal: Capture the current focused failure and identify where available
edge-publication moves leave the emission path.

Primary target:
`tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`

Actions:

- Run the focused test and capture the failure output.
- Inspect the focused fixture expectations and confirm it requires final
  prepared RV64 assembly to contain `mv a1, a0`.
- Trace the fixture through `emit_prepared_module_text(...)`,
  `append_simple_prepared_bir_function_asm(...)`, and
  `append_edge_publication_move_instruction(...)`.
- Confirm whether the simple prepared function success path bypasses available
  edge-publication move scheduling.

Completion check:

- `todo.md` records the focused failing command, the observed missing final
  assembly move, and the exact emission branch that needs repair.

### Step 2: Repair RV64 Prepared Module/Function Scheduling

Goal: Emit available prepared edge-publication moves at a semantically valid
point in final RV64 prepared assembly.

Primary target:
`src/backend/mir/riscv/codegen/prepared_module_emit.cpp`

Actions:

- Insert the smallest flow repair that schedules available edge-publication
  moves through `append_edge_publication_move_instruction(...)`.
- Keep the existing simple function emitter behavior unless moving the helper
  call into that function is the narrowest semantically correct placement.
- Preserve shared lookup authority and fail-closed helper statuses.
- Avoid any fixture-shaped matching, output-string-only shortcut, or broad
  control-flow rewrite.

Completion check:

- The focused fixture's final emitted RV64 assembly includes the prepared
  register-edge move, and unsupported/missing-authority cases remain
  fail-closed.

### Step 3: Strengthen Focused Regression Coverage If Needed

Goal: Ensure the test suite proves final assembly scheduling, not just helper
availability.

Primary target:
`tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`

Actions:

- Keep or add assertions that inspect final prepared module text for the
  emitted register-edge move.
- Preserve tests that prove the helper consumes shared prepared lookup
  authority and fails closed without it.
- Add only focused coverage needed to protect the repaired scheduling path.

Completion check:

- The focused regression would fail if helper intent is available but final
  module text omits the move.

### Step 4: Prove Focused And Narrow RV64 Backend Behavior

Goal: Validate the repair without masking unrelated backend state.

Actions:

- Run:
  `ctest --test-dir build -R '^backend_riscv_prepared_edge_publication$' --output-on-failure`
- Run a supervisor-approved narrow RV64/backend subset before acceptance.
- If the diff touches broader shared behavior, ask the supervisor to escalate
  to a broader backend or full-suite check.

Completion check:

- Focused test is green.
- The chosen narrow RV64/backend subset is monotonic.
- `todo.md` records exact proof commands and results.

## Acceptance Checklist

- `backend_riscv_prepared_edge_publication` passes.
- Final emitted RV64 assembly for the focused register-edge fixture contains
  `mv a1, a0`.
- Shared prepared lookup authority remains the source of edge-publication
  facts.
- Unsupported source/destination homes remain explicit fail-closed cases.
- The repair is in RV64 prepared module/function flow, not AArch64 code or a
  broad shared authority rewrite.
- The route does not combine or mask the separate idea 327 regression.
