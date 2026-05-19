# AArch64 Large Stack Offset Addressing Runbook

Status: Active
Source Idea: ideas/open/314_aarch64_large_stack_offset_addressing.md
Activated from: post-313 lifecycle state with no active `plan.md` or `todo.md`

## Purpose

Repair AArch64 stack-slot memory addressing and stack-backed scalar result
publication when generated frame offsets exceed the immediate range accepted by
the selected load/store instruction form.

## Goal

Make the large stack-offset representatives advance past their current
out-of-range stack addressing diagnostics through legal AArch64 address
materialization, not testcase-specific offset handling.

## Core Rule

Progress must be a general large stack-offset addressing capability for
stack-slot loads/stores or stack-backed scalar publications. Do not change
expectations, unsupported classifications, allowlists, runners, timeout policy,
proof-log contents, CTest registration, semantic handoff contracts, f128
transport behavior, or unrelated runtime behavior.

## Read First

- `ideas/open/314_aarch64_large_stack_offset_addressing.md`
- `ideas/closed/312_aarch64_lir_to_bir_local_memory_prepared_handoff.md`
- `ideas/closed/313_aarch64_f128_transport_machine_printer.md`
- `todo.md`
- existing focused `test_before.log` / `test_after.log`, when present
- generated AArch64 artifacts for `00204.c` and `00216.c` under
  `build/c_testsuite_aarch64_backend/src/`

## Current Targets

- Representative external cases:
  - `c_testsuite_aarch64_backend_src_00216_c`
  - `c_testsuite_aarch64_backend_src_00204_c`
- Current residual facts:
  - `00216.c`: backend assembly validation rejects `ldr x13, [sp, #1644]`
    because the selected stack load uses an out-of-range immediate form.
  - `00204.c`: machine printing rejects a selected scalar `add` publication
    because the scalar ALU stack-publication offset is not printable.
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
- Do not repair runtime mismatches, linker behavior, timeout behavior,
  direct-call shuffle, direct vararg, address-of-local, or unrelated frame
  layout residuals under this owner.
- Do not rely on filenames, function names, offset literals, stack-slot ids,
  scalar opcodes, block/instruction numbers, diagnostic strings, or
  c-testsuite numbers.
- Do not rework frame layout globally unless focused evidence proves legal
  large stack-offset addressing cannot be repaired at the lowering/printing
  boundary.

## Working Model

Two representatives now point to the same large-offset addressing family:

- `00216.c` reaches emitted AArch64 assembly and fails because a stack-slot
  load uses a direct `[sp, #1644]` form outside the accepted immediate range.
- `00204.c` reaches AArch64 scalar machine printing and fails before emission
  because a stack-backed scalar ALU result publication cannot print its large
  destination stack offset.

The owner should localize the stack memory operand and scalar publication
paths, choose a shared legal address-materialization strategy where possible,
and keep the repair bounded to selected large stack offsets.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Preserve frame-slot identity, width, alignment, and signedness semantics
  while materializing large stack offsets.
- Prefer local backend printer/dispatch/target-record coverage before relying
  on the external c-testsuite representatives.
- Keep normal in-range stack offsets on their existing direct paths.
- If either representative advances to an unrelated printer, assembler, linker,
  runtime, or timeout residual, record the new first bad fact and return it to
  lifecycle classification unless generated-code evidence proves this same
  owner owns it.
- Use narrow build plus focused CTest proof for implementation packets. Escalate
  to broader backend validation only when the supervisor requests it or the
  implementation touches shared target-machine behavior broadly.

## Ordered Steps

### Step 1: Localize Large Stack Offset Addressing Paths

Goal: identify the concrete AArch64 lowering/printing paths that emit or reject
large stack offsets for stack-slot loads/stores and scalar stack publications.

Primary target: AArch64 memory operand selection, stack-slot load/store
printing, and scalar ALU stack-publication printing.

Actions:

- Inspect the current `00216.c` assembly-validation residual and generated
  assembly enough to identify the selected stack load/store path.
- Inspect the current `00204.c` scalar ALU stack-publication printer residual
  enough to identify the selected publication node and destination stack fact.
- Compare both paths against local AArch64 target-instruction, dispatch, and
  machine-printer coverage.
- Record in `todo.md` whether the two residuals share one repair surface or
  need ordered substeps under this owner.

Completion check:

- `todo.md` names the missing large-offset addressing fact, the owning code
  surface, representative tests, and the smallest focused proof command for the
  repair.

### Step 2: Repair Large Stack Offset Materialization

Goal: make selected large stack offsets use legal AArch64 address
materialization instead of out-of-range direct stack immediates.

Primary target: the code surface identified by Step 1.

Actions:

- Implement a narrow large-offset materialization path for stack-slot
  loads/stores and/or scalar stack publications.
- Preserve existing direct addressing for offsets that are already encodable.
- Keep unsupported or incomplete memory operand forms fail-closed.
- Avoid semantic admission, f128 transport, runner, timeout, expectation, and
  unrelated AArch64 codegen changes.

Completion check:

- Focused backend proof shows large stack offsets are printable or emitted
  through legal sequences, or `todo.md` records the next first bad fact with
  evidence.

### Step 3: Add Focused Backend Coverage

Goal: make large stack-offset addressing behavior observable in local backend
tests.

Primary target: existing AArch64 machine-printer, instruction-dispatch, or
target-instruction-record tests.

Actions:

- Add or extend coverage for out-of-range stack-slot loads/stores.
- Add or extend coverage for stack-backed scalar ALU result publication with an
  out-of-range stack offset.
- Assert structured memory operands and legal selected behavior, not incidental
  instruction text or one literal offset.

Completion check:

- Local backend coverage exercises the large-offset contract and passes without
  weakening existing in-range stack addressing behavior.

### Step 4: Validate And Classify Residuals

Goal: prove the owner result and classify any new focused residual.

Primary target: supervisor-selected focused proof scope.

Actions:

- Run a focused proof including build, local AArch64 backend coverage,
  prior-owner guardrails from ideas 312 and 313, and the `00204.c` / `00216.c`
  c-testsuite representatives.
- Record pass/fail results and first bad facts in `todo.md`.
- Do not claim this owner complete if either current large stack-offset
  diagnostic remains.

Completion check:

- `todo.md` records fresh proof. The current large stack-offset diagnostics are
  gone for both representatives, or the remaining blocker is explicitly
  localized for the next packet.
