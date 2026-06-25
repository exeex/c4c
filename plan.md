# RV64 Object Route Frame-Slot Value Call Arguments Runbook

Status: Active
Source Idea: ideas/open/373_rv64_object_route_frame_slot_value_call_args.md

## Purpose

Lower prepared call arguments whose scalar payload value currently lives in a
frame slot or spill home in the RV64 object route.

## Goal

Admit the first semantic RV64 object-emission path for scalar frame-slot-value
call arguments, using prepared value-home, frame-slot, and call-plan metadata.

## Core Rule

Load and pass the scalar payload value selected by the prepared call plan. Do
not materialize frame-slot addresses, infer source-shape intent, or hard-code
representative-specific slots, offsets, values, or argument registers.

## Read First

- `ideas/open/373_rv64_object_route_frame_slot_value_call_args.md`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `build/agent_state/368_step3_representatives_after_pointer.log` when present

## Current Targets

- Primary representative: `src/20000121-1.c`
- Focused backend coverage:
  - `backend_riscv_object_emission`
  - `backend_prepare_frame_stack_call_contract`
  - `backend_prepared_printer`

## Non-Goals

- Frame-slot address call-argument materialization; that belongs to
  `ideas/open/372_rv64_object_route_frame_slot_address_call_args.md`.
- Pointer-value local-memory loads and stores; idea 368 closed that route.
- Aggregate `va_arg` helper lowering.
- Byval aggregate parameter homes.
- General terminator lowering, CFG reconstruction, or source-shape shortcuts.
- Expectation downgrades, allowlist filtering, or unsupported-test weakening.

## Working Model

The failing representative has advanced past pointer-value local-memory
support. The current boundary is scalar call-argument publication where the
argument payload must be recovered from a prepared frame-slot or spill home and
moved into the ABI-selected argument GPR.

The implementation should consume only explicit prepared facts:

- call argument source selection and ABI placement
- value-home metadata for the argument payload
- frame-slot id, offset, size, alignment, and stack-frame bounds
- scalar access width and sign/zero-extension needs when available

Unsupported neighbors should remain fail-closed with precise diagnostics:
missing slots, unsupported widths, bad alignments, non-GPR destinations,
out-of-range offsets, aggregate values, volatile or non-default address-space
forms, and frame-slot-address selections.

## Execution Rules

- Keep implementation changes focused in RV64 object emission and focused
  backend tests.
- Preserve existing pointer-value and frame-slot local-memory behavior.
- Add fail-closed tests beside the admitted positive fixture.
- Put probe logs under `build/agent_state/`.
- Use `test_after.log` only for the supervisor-delegated proof command.
- Do not edit the source idea unless lifecycle state or durable intent changes.

## Steps

### Step 1: Audit Frame-Slot Value Call Arguments

Goal: identify the prepared call-plan and value-home facts needed for the
representative and focused fixtures.

Actions:

- Dump or inspect prepared facts for `src/20000121-1.c`.
- Confirm the residual source is a scalar payload in a frame-slot or spill
  home, not a frame-slot address argument.
- Record the required metadata, missing facts, and first supportable form in
  `todo.md`.

Completion check:

- `todo.md` names the concrete prepared source-selection and value-home shape
  for the implementation packet.

### Step 2: Admit Scalar Frame-Slot Value Call Arguments

Goal: implement the first supportable RV64 lowering path for scalar
frame-slot-value call arguments.

Actions:

- Load the scalar payload from the prepared frame-slot home using the final
  stack-frame offset calculation already used by object emission.
- Move or extend the loaded payload into the ABI-selected GPR argument
  register according to the prepared scalar width and placement facts.
- Keep unsupported adjacent shapes fail-closed with precise diagnostics.
- Add focused positive and negative tests in
  `tests/backend/mir/backend_riscv_object_emission_test.cpp`.

Completion check:

- Focused tests prove the admitted scalar path and fail-closed unsupported
  neighbors.
- The delegated proof command passes:

```sh
cmake --build build --target c4cll backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_riscv_object_emission_test &&
ctest --test-dir build -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_riscv_object_emission)$' --output-on-failure > test_after.log
```

### Step 3: Rerun Representative

Goal: verify semantic progress on the source representative.

Actions:

- Rerun `src/20000121-1.c` through the RV64 gcc torture backend runner with a
  temporary one-case allowlist.
- Store the log under `build/agent_state/`.
- Record whether the case passes or advances to a distinct owner.

Completion check:

- `todo.md` records the representative result, next boundary if any, and proof
  artifact path.

### Step 4: Closure or Handoff Decision

Goal: decide whether idea 373 is complete or needs a narrowed follow-up.

Actions:

- Compare focused test coverage and representative evidence against the source
  idea acceptance criteria.
- If complete, request lifecycle close.
- If a distinct residual owner appears, route it to an existing or new
  `ideas/open/` child before closing or deactivating.

Completion check:

- The plan owner can either close idea 373 with regression proof or identify
  the exact remaining owner without broadening this runbook.
