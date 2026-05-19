# AArch64 Large Frame Adjustment Materialization Runbook

Status: Active
Source Idea: ideas/open/315_aarch64_large_frame_adjustment_materialization.md
Activated from: idea 314 closure after Step 4 residual classification

## Purpose

Repair AArch64 frame setup and teardown printing when a function frame size is
outside the plain `sub/add sp, sp, #imm` encoding range.

## Goal

Make large AArch64 frame adjustments use legal stack-pointer materialization
sequences while preserving direct output for encodable frame sizes.

## Core Rule

Progress must be a general large frame setup/teardown materialization
capability. Do not reopen stack-slot memory spelling, scalar stack publication,
frame-layout consistency, semantic handoff, f128 transport, expectations,
unsupported classifications, runners, timeout policy, proof-log contents, or
CTest registration.

## Read First

- `ideas/open/315_aarch64_large_frame_adjustment_materialization.md`
- `ideas/closed/314_aarch64_large_stack_offset_addressing.md`
- `ideas/closed/312_aarch64_lir_to_bir_local_memory_prepared_handoff.md`
- `ideas/closed/313_aarch64_f128_transport_machine_printer.md`
- `todo.md`
- existing focused `test_before.log` / `test_after.log`, when present
- generated AArch64 artifacts and prepared dumps for `00204.c` under
  `build/c_testsuite_aarch64_backend/src/`

## Current Targets

- Representative external case:
  - `c_testsuite_aarch64_backend_src_00204_c`
- Current residual fact:
  - `00204.c`: machine printing rejects
    `family=frame opcode=frame_setup` because `frame_size=5776` is outside the
    plain frame adjustment immediate range.
- Prior-owner guardrails:
  - `backend_lir_to_bir_notes`
  - `backend_cli_dump_bir_00204_stdarg_semantic_handoff`
  - `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff`
  - `backend_cli_dump_bir_focus_function_filters_00204`
  - `backend_cli_dump_prepared_bir_focus_function_filters_00204`
  - `backend_cli_dump_prepared_bir_focus_block_entry_00204`
  - `backend_aarch64_machine_printer`
  - `backend_aarch64_instruction_dispatch`
  - `backend_aarch64_target_instruction_records`

## Non-Goals

- Do not reopen idea 312's semantic local-memory prepared-handoff owner.
- Do not reopen idea 313's `f128_transport` printer owner.
- Do not reopen idea 314's stack-slot load/store or scalar stack-publication
  instruction-spelling owner.
- Do not repair `00216.c` frame-slot/frame-layout consistency under this owner.
- Do not repair runtime mismatches, linker behavior, timeout behavior,
  direct-call shuffle, direct vararg, or address-of-local residuals under this
  owner.
- Do not rely on filenames, function names, literal `5776`, stack-slot ids,
  block/instruction numbers, diagnostic strings, or c-testsuite numbers.
- Do not rework frame layout unless focused evidence proves the frame-size
  value itself is wrong.

## Working Model

Idea 314 legalized selected stack-slot memory operands and stack-backed scalar
publication. `00204.c` now fails in the frame instruction printer:
`print_frame` rejects `frame.frame_size_bytes > 4095` before printing the
function body. The owner should localize frame setup and teardown printing,
materialize large stack-pointer adjustments through legal AArch64 sequences,
and keep in-range frames on their existing direct path.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Preserve stack-pointer balance between setup and teardown.
- Prefer local backend printer/dispatch/target-record coverage before relying
  on the external c-testsuite representative.
- Keep normal in-range frame adjustments on their existing direct path.
- Treat stack-slot/frame-layout divergence as idea 316, not this owner.
- If the representative advances to an unrelated printer, assembler, linker,
  runtime, or timeout residual, record the new first bad fact and return it to
  lifecycle classification unless generated-code evidence proves this same
  owner owns it.
- Use narrow build plus focused CTest proof for implementation packets.
  Escalate to broader backend validation only when the supervisor requests it
  or the implementation touches shared target-machine behavior broadly.

## Ordered Steps

### Step 1: Localize Frame Adjustment Paths

Goal: identify the concrete AArch64 lowering/printing paths that emit or reject
large frame setup and teardown adjustments.

Primary target: AArch64 frame setup/teardown printing and any target records
that constrain frame adjustment immediates.

Actions:

- Inspect the current `00204.c` frame setup printer residual and prepared dump
  enough to confirm the frame-size fact and failing printer path.
- Inspect frame teardown behavior so setup and teardown are repaired together.
- Compare both paths against local AArch64 target-instruction, dispatch, and
  machine-printer coverage.
- Record in `todo.md` the owning code surface, representative tests, and the
  smallest focused proof command for the repair.

Completion check:

- `todo.md` names the missing large frame-adjustment materialization fact, the
  owning code surface, representative tests, and the smallest focused proof
  command for the repair.

### Step 2: Repair Large Frame Adjustment Materialization

Goal: make large frame setup and teardown adjustments use legal AArch64
materialization instead of rejecting out-of-range plain immediates.

Primary target: the code surface identified by Step 1.

Actions:

- Implement a narrow materialization path for frame setup and teardown.
- Preserve existing direct frame adjustment output for encodable frame sizes.
- Preserve stack-pointer balance and fail closed for unsupported forms.
- Avoid semantic admission, f128 transport, runner, timeout, expectation, and
  unrelated AArch64 codegen changes.

Completion check:

- Focused backend proof shows large frame adjustments are printable or emitted
  through legal sequences, or `todo.md` records the next first bad fact with
  evidence.

### Step 3: Add Focused Backend Coverage

Goal: make large frame adjustment behavior observable in local backend tests.

Primary target: existing AArch64 machine-printer, instruction-dispatch, or
target-instruction-record tests.

Actions:

- Add or extend coverage for large frame setup adjustment.
- Add or extend coverage for matching large frame teardown adjustment.
- Assert stack-pointer balance and legal selected behavior, not incidental
  one-literal output.

Completion check:

- Local backend coverage exercises the large frame-adjustment contract and
  passes without weakening existing in-range frame adjustment behavior.

### Step 4: Validate And Classify Residuals

Goal: prove the owner result and classify any new focused residual.

Primary target: supervisor-selected focused proof scope.

Actions:

- Run a focused proof including build, local AArch64 backend coverage,
  prior-owner guardrails from ideas 312, 313, and 314, and the `00204.c`
  c-testsuite representative.
- Record pass/fail results and first bad facts in `todo.md`.
- Do not claim this owner complete if the current frame setup/teardown
  materialization diagnostic remains.

Completion check:

- `todo.md` records fresh proof. The current large frame-adjustment diagnostic
  is gone, or the remaining blocker is explicitly localized for the next
  packet.
