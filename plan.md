# BIR Call Argument ABI Scalar Metadata Coherence Plan

Status: Active
Source Idea: ideas/open/434_bir_call_arg_abi_scalar_metadata_coherence.md

## Purpose

Repair or fail closed on raw and prepared BIR call-argument ABI metadata that
publishes aggregate-only `sret` or `byval` facts on scalar call arguments.

## Goal

Make scalar call arguments stop appearing as aggregate ABI lowering candidates
because of incoherent `bir::CallInst::arg_abi` metadata.

## Core Rule

Fix the producer contract. Do not hide the problem in RV64 target lowering, do
not rely on later prepared call-plan repair as proof of raw BIR coherence, and
do not special-case the representative testcase or function names.

## Read First

- `ideas/open/434_bir_call_arg_abi_scalar_metadata_coherence.md`
- `build/agent_state/431_step1_aggregate_abi_audit/evidence.md`
- `build/agent_state/431_step1_aggregate_abi_audit/classification.tsv`
- `build/agent_state/431_step2_abi_fact_coherence/classification.md`
- `src/backend/bir/lir_to_bir/calling.cpp`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir_printer.cpp`
- `src/backend/prealloc/legalize.cpp`
- `src/backend/prealloc/regalloc/call_return_abi.cpp`
- `src/backend/prealloc/prepared_contract_verifier.cpp`
- `tests/backend/bir/backend_prealloc_prepared_contract_verifier_test.cpp`
- `tests/backend/bir/backend_prepare_structured_context_test.cpp`

## Current Targets

- Raw `bir::CallInst::arg_abi` entries whose argument type is scalar but whose
  ABI metadata claims `sret_pointer` or `byval_copy`.
- Representative evidence:
  - `src/20010224-1.c`: scalar `i16` immediate argument rendered as
    `byval(size=14, align=8462384025005154658)`.
  - `src/20020506-1.c`: scalar `i16` register arguments rendered as
    `sret(size=5, align=...)`.
- Producer and verifier surfaces that can express scalar-vs-aggregate ABI
  coherence before target lowering consumes call facts.

## Non-Goals

- RV64 aggregate `sret` or `byval` lowering.
- Using prepared call-plan scalar repair as the only fix while leaving raw BIR
  ABI metadata incoherent.
- Named-case fixes for `ba_compute_psd`, `test3`, `test4`, `20010224-1`, or
  `20020506-1`.
- Expectation, allowlist, unsupported-marker, runtime-comparison, or pass/fail
  accounting changes.
- F128, long-double, runtime-helper, inline-asm, select, or store-publication
  residuals.

## Working Model

- `bir_printer.cpp` renders `sret(...)` and `byval(...)` call suffixes directly
  from `CallInst::arg_abi`.
- Existing prealloc repair paths can consume the affected scalar arguments as
  scalar GPR movements, but that happens after raw BIR has already published
  misleading aggregate ABI metadata.
- The durable fix should either prevent scalar arguments from receiving
  aggregate-only flags at production time or introduce a producer-owned
  verifier/classifier contract that rejects such metadata before RV64 lowering.

## Execution Rules

- Prefer a focused fail-closed contract before changing broad ABI production.
- Preserve valid aggregate `sret`/`byval` metadata for pointer aggregate
  arguments.
- Keep proof focused on backend/BIR producer contracts first; broaden only if
  the implementation touches shared ABI/lowering paths.
- If the first packet discovers the defect belongs to a narrower producer
  surface, update `todo.md` with the exact owner instead of broadening the
  plan.
- For code/test packets, use:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Steps

### Step 1: Audit Scalar Arg ABI Producer And Contract Surface

Trace where scalar call arguments receive `CallArgAbiInfo` in BIR lowering and
where raw/prepared verifier coverage could reject aggregate flags on scalar
arguments. Confirm whether the first executable packet should repair the
producer, add a raw-call-ABI verifier/classifier, or both.

Completion signal: `todo.md` names the exact producer/verifier surface, the
representative proof inputs, and the first implementation packet.

### Step 2: Add Fail-Closed Scalar ABI Coherence Coverage

Add or extend focused backend/BIR contract coverage so scalar arguments carrying
aggregate-only `sret_pointer` or `byval_copy` metadata are rejected or
classified before target lowering.

Completion signal: focused tests fail before the repair or directly exercise
the new fail-closed verifier path, and the backend proof command passes after
the change.

### Step 3: Repair Raw Call Argument ABI Production

Change the producer path so scalar call arguments do not publish aggregate-only
ABI metadata. Preserve valid pointer aggregate `sret` and `byval` call facts.

Completion signal: representative raw/prepared BIR no longer renders bogus
aggregate suffixes for scalar `i16` arguments, while coherent aggregate rows
still retain their ABI metadata.

### Step 4: Close-Readiness Review

Validate the source idea acceptance criteria: scalar arguments no longer appear
as aggregate ABI candidates due to bogus raw `arg_abi`, focused coverage exists,
and any remaining residuals are routed separately.

Completion signal: lifecycle review can close this idea or keep it active with
an exact remaining producer-contract packet.
