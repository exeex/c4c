# AArch64 F128 Transport Machine Printer Runbook

Status: Active
Source Idea: ideas/open/313_aarch64_f128_transport_machine_printer.md
Activated from: post-312 lifecycle state with no active `plan.md` or `todo.md`

## Purpose

Repair the AArch64 target machine-node path for structured `f128_transport`
nodes so valid binary128/f128 transport operations reach printable or lowered
AArch64 forms without weakening semantic prepared-handoff contracts.

## Goal

Make the focused representative advance past the current `f128_transport`
machine-printer diagnostic through structured target facts and focused backend
coverage, not c-testsuite-specific matching.

## Core Rule

Progress must be an AArch64 `f128_transport` machine-node printing or lowering
capability. Do not change expectations, unsupported classifications,
allowlists, runners, timeout policy, proof-log contents, CTest registration,
semantic admission contracts, or unrelated runtime behavior.

## Read First

- `ideas/open/313_aarch64_f128_transport_machine_printer.md`
- `ideas/closed/312_aarch64_lir_to_bir_local_memory_prepared_handoff.md`
- `todo.md`
- existing focused `test_before.log` / `test_after.log`, when present
- generated AArch64 artifacts for `00204.c` under
  `build/c_testsuite_aarch64_backend/src/`

## Current Targets

- Representative external case:
  - `c_testsuite_aarch64_backend_src_00204_c`
- Existing semantic/prepared handoff guardrails from the prior owner:
  - `backend_lir_to_bir_notes`
  - `backend_cli_dump_bir_00204_stdarg_semantic_handoff`
  - `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff`
  - `backend_cli_dump_bir_focus_function_filters_00204`
  - `backend_cli_dump_prepared_bir_focus_function_filters_00204`
  - `backend_cli_dump_prepared_bir_focus_block_entry_00204`
- Likely local backend proof buckets:
  - `backend_aarch64_machine_printer`
  - `backend_aarch64_instruction_dispatch`
  - `backend_aarch64_target_instruction_records`

## Non-Goals

- Do not reopen idea 312's semantic local-memory prepared-handoff owner.
- Do not reopen closed local-memory, global projection, call-boundary, scalar
  machine-node, or f128 redistribution owners by pass count alone.
- Do not fold in large stack-offset assembly validation, runtime mismatch,
  linker, timeout, direct-call, vararg, address-of-local, `00164.c`, or
  `00214.c` residuals.
- Do not rely on filename, function name, block number, instruction number,
  diagnostic string, or c-testsuite number special cases.
- Do not perform broad AArch64 codegen or semantic admission rewrites unless
  focused evidence proves the `f128_transport` machine-node contract requires
  that boundary.

## Working Model

Idea 312 advanced `00204.c` past the semantic local-memory prepared-handoff
owner. The current first bad fact is now AArch64 machine-node printing:

```text
cannot print AArch64 machine node family=f128_transport opcode=f128_transport:
f128 transport printer requires structured ...
```

This runbook should first localize what structured operand, value-home, width,
or lowering fact the printer requires. Implementation should then repair the
target-machine contract and prove it with local backend coverage before using
the external c-testsuite representative as acceptance evidence.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Preserve fail-closed behavior for unsupported or incomplete f128 transport
  forms.
- Prefer local backend printer/dispatch coverage before relying on
  `c_testsuite_aarch64_backend_src_00204_c`.
- If `00204.c` advances to assembler, linker, runtime, or timeout residuals,
  record the new first bad fact and return it to lifecycle classification
  unless generated-code evidence proves this same owner owns it.
- Use narrow build plus focused CTest proof for implementation packets. Escalate
  to broader backend validation only when the supervisor requests it or the
  implementation touches shared target-machine behavior broadly.

## Ordered Steps

### Step 1: Localize F128 Transport Printer Requirements

Goal: identify the exact structured operand or lowering fact missing from the
current AArch64 `f128_transport` printer failure.

Primary target: AArch64 machine-node creation, dispatch, and printer handling
for `family=f128_transport`.

Actions:

- Inspect the current `00204.c` focused failure and generated artifacts enough
  to identify the failing machine-node shape.
- Compare existing AArch64 f128/binary128 lowering, target-instruction records,
  dispatch, and printer expectations.
- Record the localized first bad fact and owning code surface in `todo.md`.

Completion check:

- `todo.md` names the missing structured `f128_transport` fact, the likely code
  owner, and the smallest local backend test surface that can prove the repair.

### Step 2: Repair F128 Transport Printing Or Lowering

Goal: make valid structured AArch64 `f128_transport` nodes lower or print
through the intended target path.

Primary target: the code surface identified by Step 1.

Actions:

- Implement the narrow target-machine repair without c-testsuite filename
  matching.
- Keep unsupported or incomplete `f128_transport` forms fail-closed.
- Avoid semantic admission, runner, timeout, expectation, and unrelated AArch64
  codegen changes.

Completion check:

- Focused backend proof shows the repaired structured `f128_transport` shape is
  printable or lowered, or `todo.md` records the next first bad fact with
  evidence.

### Step 3: Add Focused Backend Coverage

Goal: make the repaired `f128_transport` machine-node contract observable in
local backend tests.

Primary target: existing AArch64 machine-printer, instruction-dispatch, or
target-instruction-record tests.

Actions:

- Add or extend focused coverage for the structured `f128_transport` shape.
- Assert structured operands and selected target behavior, not incidental
  textual coincidences.
- Keep the external `00204.c` representative as an integration proof, not the
  only evidence for the repair.

Completion check:

- Local backend coverage fails before the repair or directly exercises the new
  contract, and passes after the repair without weakening existing contracts.

### Step 4: Validate And Classify Residuals

Goal: prove the owner result and classify any new focused residual.

Primary target: supervisor-selected focused proof scope.

Actions:

- Run a focused proof including build, local AArch64 backend coverage, the
  semantic/prepared guardrails from idea 312, and
  `c_testsuite_aarch64_backend_src_00204_c`.
- Record pass/fail results and first bad facts in `todo.md`.
- Do not claim this owner complete if the old `f128_transport` printer
  diagnostic remains.

Completion check:

- `todo.md` records fresh proof. The old `f128_transport` printer diagnostic is
  gone for the representative, or the remaining blocker is explicitly
  localized for the next packet.
