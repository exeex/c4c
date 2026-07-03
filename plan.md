# RV64 Pointer-Result Frame-Slot Address Materialization Runbook

Status: Active
Source Idea: ideas/open/568_rv64_pointer_result_frame_slot_address_materialization.md

## Purpose

Repair the RV64 object-emission gap for prepared pointer-result local address
materialization, starting from the pinned `bir.add ptr` fragment in
`src/20001026-1.c`.

## Goal

Consume prepared address-materialization facts and value-home authority so a
pointer base plus dynamic integer byte offset can produce a pointer result in
the prepared destination home, including frame-slot destinations.

## Core Rule

Do not re-open the div/rem route. Idea 567 proved raw div/rem opcode lowering
already exists; this runbook owns only the downstream pointer-result address
materialization fallthrough.

## Read First

- `ideas/open/568_rv64_pointer_result_frame_slot_address_materialization.md`
- `ideas/closed/567_rv64_integer_div_rem_instruction_fragment_lowering.md`
- `build/agent_state/567_step2_pinned_fragment.md`
- `build/agent_state/567_step1_20001026-1.bir.txt`
- `build/agent_state/567_step1_20001026-1.prepared_bir.txt`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/prepared_scalar_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_frame_emit.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `scripts/check_progress_rv64_gcc_c_torture_backend.sh`

## Current Targets

- Representative case: `src/20001026-1.c`
- Function/block: `real_value_from_int_cst`, `block_1`
- Pinned instruction: `%t12 = bir.add ptr %lv.r.0, %t12.byte_offset`
- Traversal location: block index `3`, instruction index `4`
- Base value: `%lv.r.0`, value id `14`, GPR register home `s1`,
  placement `gpr:callee_saved#0/w1`
- Offset value: `%t12.byte_offset`, value id `12`, frame-slot home
  `slot#22+stack80`
- Result value: `%t12`, value id `13`, pointer frame-slot home
  `slot#23+stack88`
- Prepared fact: `address_materialization block=block_1 inst_index=4
  kind=frame_slot result=%lv.r.0 offset=8`
- Current rejection: generic `unsupported_instruction_fragment` after
  `fragment_for_prepared_instruction(...)` does not claim the pointer-result
  binary instruction.

## Non-Goals

- Do not add or duplicate raw `sdiv`, `udiv`, `srem`, or `urem` opcode
  lowering.
- Do not implement broad pointer/integer cast cleanup outside the pinned
  pointer-result address-materialization contract.
- Do not touch F128, scalar FPR, ABI/call, runtime, or producer/prepared
  admission policy.
- Do not rewrite pass/fail expectations, unsupported markers, or allowlists as
  a substitute for lowering support.
- Do not redesign local memory beyond what is required to consume the prepared
  address-materialization facts for this pointer-result fragment.

## Working Model

The prepared traversal already carries enough facts to identify a pointer
address-materialization event at the same source block and instruction index
as the unsupported `bir.add ptr`. The implementation should validate those
facts, materialize the pointer expression with existing RV64 register and
stack-offset helpers, and publish the result into the prepared destination
home. Missing or incoherent facts must leave the existing unsupported behavior
in place.

## Execution Rules

- Match semantic facts and value homes, not testcase names, `%t12`, diagnostic
  strings, or raw instruction text.
- Keep the implementation narrow to pointer-result local/frame-slot address
  materialization from a pointer base plus integer byte offset.
- Preserve direct unsupported behavior for malformed, missing, or ambiguous
  prepared facts.
- Add focused tests before or with the lowering change, including fail-closed
  coverage for incoherent facts.
- Prove the representative allowlist containing `src/20001026-1.c` advances
  past the pinned generic unsupported fragment.
- If the representative row reaches a new residual, record the concrete
  downstream owner in `todo.md` instead of expanding this runbook.

## Step 1: Reconstruct Pointer Address Boundary

Goal: Confirm the prepared facts, value homes, and current rejecting hook for
the pointer-result frame-slot address-materialization shape.

Primary targets:

- `build/agent_state/567_step2_pinned_fragment.md`
- `build/agent_state/567_step1_20001026-1.bir.txt`
- `build/agent_state/567_step1_20001026-1.prepared_bir.txt`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/prepared_scalar_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_frame_emit.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`

Actions:

- Re-read the pinned fragment artifact and the BIR/prepared-BIR dumps.
- Identify the object-emission helper that should own pointer-result
  address-materialization, or the smallest helper extraction needed.
- Record the exact prepared fact lookup key, base/offset/result value-home
  expectations, and current fallthrough path in `todo.md`.
- Inspect nearby object-emission tests for existing frame-address,
  stack-offset, and result-publication coverage that can be reused.
- Do not edit implementation files in this step unless the supervisor
  explicitly delegates a code packet.

Completion check:

- `todo.md` names the semantic lowering hook, required prepared facts,
  destination publication rule, and focused coverage target for Step 2.

## Step 2: Add Focused Pointer-Result Coverage

Goal: Pin the expected behavior for coherent pointer-result address
materialization and fail-closed behavior for malformed prepared facts.

Primary targets:

- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- Existing RV64 prepared object-emission helpers used by nearby frame-address
  and stack-slot tests

Actions:

- Add focused tests for pointer base plus dynamic integer byte offset producing
  a pointer result in a frame-slot destination.
- Include the pinned shape's key facts: pointer base in a GPR home, offset in a
  stack/frame-slot-backed integer home, and pointer result in a frame slot.
- Add malformed or missing prepared-fact coverage that preserves unsupported
  behavior.
- Keep tests semantic; do not mention `src/20001026-1.c`, `%t12`, or the
  diagnostic string as the matching key.

Completion check:

- The focused tests fail before implementation or are otherwise shown to
  exercise the newly repaired branch.
- `cmake --build --preset default` succeeds far enough to compile the tests.

## Step 3: Implement Prepared Pointer-Result Materialization

Goal: Lower the validated pointer-result address-materialization shape in RV64
prepared object emission.

Primary targets:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/prepared_scalar_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_frame_emit.cpp`

Actions:

- Validate the prepared address-materialization fact against the current
  instruction location and value-home authority.
- Materialize the pointer base and dynamic integer byte offset with existing
  operand/home helpers.
- Publish the computed pointer result into the prepared destination home,
  including frame-slot destinations.
- Leave missing, incoherent, non-local-memory, or non-pointer-result shapes on
  the existing unsupported path.
- Avoid broad local-memory, pointer-cast, or producer/prepared rewrites.

Completion check:

- Focused coverage from Step 2 passes.
- `cmake --build --preset default` and the supervisor-selected backend subset
  pass.

## Step 4: Prove Representative Progress

Goal: Show that the representative row advances past the pinned pointer-result
generic unsupported fragment.

Primary targets:

- `scripts/check_progress_rv64_gcc_c_torture_backend.sh`
- Representative allowlist under `build/agent_state/`
- Current `test_after.log` for backend subset proof

Actions:

- Run the supervisor-delegated backend proof command.
- Run a representative allowlist containing at least `src/20001026-1.c`.
- Confirm the pinned generic unsupported fragment for
  `%t12 = bir.add ptr %lv.r.0, %t12.byte_offset` is gone.
- Record any new residual diagnostic and concrete downstream owner in
  `todo.md`.
- If the old fragment remains, do not accept the slice as progress.

Completion check:

- `todo.md` records backend proof, representative allowlist proof, old
  fragment removal, and any downstream residual owner.

## Step 5: Closure Check And Residual Routing

Goal: Decide whether the source idea is complete or needs another narrowed
packet under the same pointer-result address-materialization owner.

Actions:

- Compare the implementation and proof against the source idea acceptance
  criteria.
- Confirm focused tests cover coherent and fail-closed cases.
- Confirm the representative old diagnostic is gone without expectation
  rewrites or testcase-shaped matching.
- If a new residual is outside this idea, record it as a downstream route
  instead of extending this runbook.
- Ask the plan owner to close only when the source idea itself is satisfied.

Completion check:

- The source idea is either ready for closure with regression guard proof, or
  `todo.md` identifies the precise remaining pointer-result materialization
  packet.
