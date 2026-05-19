# AArch64 Memory Store Operand Materialization Runbook

Status: Active
Source Idea: ideas/open/301_aarch64_memory_store_operand_materialization.md
Activated: 2026-05-19
Supersedes: ideas/open/295_backend_regex_failure_family_inventory.md active umbrella runbook

## Purpose

Implement the focused post-300 memory-store machine-printer owner split from
the backend-regex umbrella inventory.

## Goal

Make AArch64 memory stores publish or materialize printable source, address,
and symbolic value operands before machine printing.

## Core Rule

Repair the semantic store operand path. Do not claim progress through
expectation, allowlist, unsupported-classification, timeout, runner, or CTest
registration changes.

## Read First

- `ideas/open/301_aarch64_memory_store_operand_materialization.md`
- `ideas/open/295_backend_regex_failure_family_inventory.md`
- `test_before.log`, as the accepted post-300 backend-regex inventory:
  352 selected, 298 passed, and 54 failed

## Current Targets

- Focused cases: `00173`, `00176`, `00181`, `00182`, `00187`, `00194`,
  `00213`, and `00214`.
- Store source scratch not printable: `00173`, `00187`, `00194`.
- Symbol/global store value not represented as register/immediate:
  `00176`, `00181`, `00182`, `00213`, and `00214`.

## Non-Goals

- Do not reopen closed owners 285 through 300 from residual counts alone.
- Do not fold scalar mul/div/rem, call-boundary, unsigned reduction,
  `lir_to_bir`, invalid scalar cast spelling, runtime nonzero,
  runtime mismatch/crash, or timeout residuals into this owner without new
  generated-code or diagnostic evidence.
- Do not change tests, expectations, allowlists, unsupported classifications,
  timeout policy, runner behavior, or CTest registration.
- Do not match c-testsuite filenames or exact diagnostic strings as the fix.

## Working Model

- The post-300 umbrella inventory identified eight compile-blocking memory
  store diagnostics as the next coherent owner.
- The relevant failure mode is that a store operand reaches the AArch64
  machine printer without being represented as a printable structured operand.
- A valid fix may live in lowering, operand publication, selected-node
  normalization, materialization, or printer admission, but it must make the
  store instruction semantically printable instead of hiding the diagnostic.
- Runtime residuals are not acceptance evidence for this owner unless a probe
  first proves they share the same store operand materialization failure.

## Execution Rules

- Keep packet progress and proof notes in `todo.md`.
- Before editing code, inspect at least one case from each focused subfamily:
  source scratch and symbol/global store value.
- Prefer small semantic changes with generated-assembly or diagnostic proof.
- After each implementation slice, run the supervisor-delegated build and
  focused backend c-testsuite subset exactly as requested.
- Broader backend-regex proof is required before accepting this focused owner
  as complete.

## Ordered Steps

### Step 1: Confirm store operand failure shapes

Goal: prove the focused cases share memory-store operand materialization or
publication failures.

Actions:

- Inspect diagnostics or generated intermediate/machine output for
  `00173`, `00187`, and `00194` to identify the unprintable store source
  scratch form.
- Inspect diagnostics or generated intermediate/machine output for
  `00176`, `00181`, `00182`, `00213`, and `00214` to identify the
  symbol/global store value form that is not a register or immediate.
- Identify the earliest shared lowering, selected-node, or printer-admission
  boundary where the operands should become structured printable forms.
- Record the confirmed boundary and any cases that fall outside this owner in
  `todo.md`.

Completion check:

- `todo.md` names the shared store operand boundary, the cases still in scope,
  and the narrow proof command the executor will use for the first code slice.

### Step 2: Materialize store source scratch operands

Goal: make stack/local memory-store source operands printable without
case-specific handling.

Actions:

- Repair the semantic path that leaves source scratch values unstructured for
  memory stores.
- Ensure the selected AArch64 store form receives a register, immediate, or
  other valid structured operand accepted by the printer.
- Run fresh build proof and the focused subset delegated by the supervisor.

Completion check:

- `00173`, `00187`, and `00194` no longer fail from the old unprintable store
  source scratch diagnostic, and no unrelated focused case regresses.

### Step 3: Materialize symbol/global store value operands

Goal: make stores of symbolic or global values publish printable operands.

Actions:

- Repair the semantic path that leaves symbol/global store values outside the
  register/immediate operand forms accepted by AArch64 store printing.
- Reuse the same materialization or publication model from Step 2 when it
  applies; split only if generated-code evidence proves a distinct subfamily.
- Run fresh build proof and the focused subset delegated by the supervisor.

Completion check:

- `00176`, `00181`, `00182`, `00213`, and `00214` no longer fail from the old
  store symbol/value printer diagnostic, and no unrelated focused case
  regresses.

### Step 4: Focused closure and broader backend-regex proof

Goal: decide whether the focused owner is complete and safe to close.

Actions:

- Rerun the full focused memory-store subset after all store operand fixes.
- Run the supervisor-selected broader backend-regex proof.
- Separate remaining runtime, timeout, scalar, `lir_to_bir`, and other
  residual buckets from this owner.
- Update `todo.md` with final proof and any remaining out-of-scope residuals
  before requesting lifecycle closure.

Completion check:

- The old memory-store operand printer diagnostics are absent from focused
  proof, broader proof is recorded, and any residual focused failures are
  classified outside this owner with evidence.
