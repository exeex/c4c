# RV64 Object Route Frame-Slot Address Call Arguments Runbook

Status: Active
Source Idea: ideas/open/372_rv64_object_route_frame_slot_address_call_args.md

## Purpose

Lower prepared call arguments whose source is a local frame-slot address in the
RV64 object route.

## Goal

Admit the first semantic RV64 object-emission path for materializing a prepared
frame-slot address into an ABI-selected GPR call argument.

## Core Rule

Pass the address selected by the prepared call plan. Do not load or pass the
frame-slot payload value, infer source-shape intent, or hard-code
representative-specific slots, offsets, values, or argument registers.

## Read First

- `ideas/open/372_rv64_object_route_frame_slot_address_call_args.md`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `build/agent_state/368_step3_representatives_after_pointer.log` when present

## Current Targets

- Primary representatives:
  - `src/20000217-1.c`
  - `src/va-arg-13.c`
- Focused backend coverage:
  - `backend_riscv_object_emission`
  - `backend_prepare_frame_stack_call_contract`
  - `backend_prepared_printer`

## Non-Goals

- Pointer-value local-memory loads and stores; idea 368 closed that route.
- Frame-slot payload-value call arguments; idea 373 closed that route.
- Non-register parameter homes; use
  `ideas/open/374_rv64_object_route_non_register_param_homes.md`.
- Aggregate `va_arg` helper lowering.
- Byval aggregate parameter homes.
- General terminator lowering, CFG reconstruction, or source-shape shortcuts.
- Expectation downgrades, allowlist filtering, or unsupported-test weakening.

## Working Model

The known residuals require publishing a stack-slot address, not a scalar
payload, into a call-argument GPR. The implementation should consume only
explicit prepared facts:

- call argument source selection and ABI placement
- frame-slot id and local address-materialization metadata
- stack-frame size and final slot offset calculation
- constant source offset when present
- GPR destination register selected by the prepared call plan

Unsupported neighbors should remain fail-closed with precise diagnostics:
missing frame slots, dynamic stack layouts, out-of-range offsets, non-default
address spaces, TLS or aggregate address forms, non-GPR destinations, and
payload-value call selections.

## Execution Rules

- Keep implementation changes focused in RV64 object emission and focused
  backend tests.
- Preserve existing pointer-value local-memory and frame-slot payload-value
  call-argument behavior.
- Add fail-closed tests beside the admitted positive fixture.
- Put probe logs under `build/agent_state/`.
- Use `test_after.log` only for the supervisor-delegated proof command.
- Do not edit the source idea unless lifecycle state or durable intent changes.

## Steps

### Step 1: Audit Frame-Slot Address Call Arguments

Goal: identify the prepared call-plan and frame-slot facts needed for the
representatives and focused fixtures.

Actions:

- Dump or inspect prepared facts for `src/20000217-1.c` and `src/va-arg-13.c`.
- Confirm which residuals are frame-slot address call arguments and which are
  blocked earlier by non-register parameter homes.
- Record the required metadata, missing facts, and first supportable form in
  `todo.md`.

Completion check:

- `todo.md` names the concrete prepared address-materialization shape for the
  implementation packet and routes any earlier blockers to the correct owner.

### Step 2: Admit Frame-Slot Address GPR Call Arguments

Goal: implement the first supportable RV64 lowering path for prepared
frame-slot address call arguments.

Actions:

- Materialize the final stack-slot address using prepared frame-slot metadata
  and the function frame-plan size.
- Write the address into the ABI-selected GPR argument register before the
  call.
- Keep unsupported adjacent shapes fail-closed with precise diagnostics.
- Add focused positive and negative tests in
  `tests/backend/mir/backend_riscv_object_emission_test.cpp`.

Completion check:

- Focused tests prove the admitted frame-slot address path and fail-closed
  unsupported neighbors.
- The delegated proof command passes:

```sh
cmake --build build --target c4cll backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_riscv_object_emission_test &&
ctest --test-dir build -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_riscv_object_emission)$' --output-on-failure > test_after.log
```

### Step 3: Rerun Representatives

Goal: verify semantic progress on the listed source representatives.

Actions:

- Rerun `src/20000217-1.c` and `src/va-arg-13.c` through the RV64 gcc torture
  backend runner with a temporary allowlist.
- Store logs under `build/agent_state/`.
- Record whether each case passes, advances, or remains blocked by an existing
  out-of-scope owner.

Completion check:

- `todo.md` records each representative result, next boundary if any, and
  proof artifact paths.

### Step 4: Closure or Handoff Decision

Goal: decide whether idea 372 is complete or needs a narrowed follow-up.

Actions:

- Compare focused test coverage and representative evidence against the source
  idea acceptance criteria.
- If complete, request lifecycle close.
- If a distinct residual owner appears, route it to an existing or new
  `ideas/open/` child before closing or deactivating.

Completion check:

- The plan owner can either close idea 372 with regression proof or identify
  the exact remaining owner without broadening this runbook.
