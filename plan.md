# RV64 Coherent Aggregate SRet Call Storage Lowering Plan

Status: Active
Source Idea: ideas/open/435_rv64_coherent_aggregate_sret_call_storage_lowering.md

## Purpose

Lower coherent RV64 aggregate `sret` call-storage facts using prepared ABI
metadata for rows that survived producer review. This plan starts from
`src/20000917-1.c` and `src/20020206-1.c` and excludes producer-defect rows.

## Goal

Consume prepared aggregate `sret` memory-return facts to remove the aggregate
`sret` unsupported fragment, or reduce it to a smaller correctly classified
non-aggregate residual, without target-side ABI guessing.

## Core Rule

Prepared memory-return facts are the authority. RV64 lowering must not infer
aggregate size, alignment, field offsets, return slots, storage homes, or
byval-copy homes from source text, raw BIR shape, function names, or testcase
identity.

## Read First

- `ideas/open/435_rv64_coherent_aggregate_sret_call_storage_lowering.md`
- `ideas/closed/431_prepared_aggregate_abi_contract_review.md`
- `ideas/closed/434_bir_call_arg_abi_scalar_metadata_coherence.md`
- `build/agent_state/431_step1_aggregate_abi_audit/evidence.md`
- `build/agent_state/431_step1_aggregate_abi_audit/classification.tsv`
- `build/agent_state/431_step2_abi_fact_coherence/classification.md`
- `src/backend/mir/riscv/codegen/prepared_call_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_frame_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_local_memory_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_scalar_emit.cpp`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `tests/backend/case/riscv64_byval_aggregate_fixed_call.c`
- `tests/backend/case/riscv64_byval_aggregate_float_lane.c`

## Current Targets

- `src/20000917-1.c`: coherent `sret(size=12, align=4)` aggregate returns for
  `one` and `zero`, with `memory_return`, `memory_home=frame_slot`,
  `sret_arg=0`, and field-copy facts at offsets `0`, `4`, and `8`.
- `src/20020206-1.c`: coherent `sret(size=12, align=4)` aggregate return for
  `bar`, with the same prepared memory-return authority.
- RV64 caller-side frame-slot memory-return setup and post-call aggregate value
  materialization from prepared facts.

## Non-Goals

- Repairing raw BIR scalar call ABI metadata defects; idea 434 closed that
  producer path.
- Reviewing or consuming `src/pr88904.c` before idea 436 reconciles its
  size/alignment/object-home facts.
- Including `src/20010224-1.c` or `src/20020506-1.c` as coherent aggregate
  target-lowering proof.
- Scalar call publication, inline-asm, select lowering, pointer residuals,
  F128, long-double, runtime-helper policy, or expectation/pass-fail
  accounting work.

## Working Model

- The coherent rows already publish prepared callsites with `memory_return`,
  `memory_home=frame_slot`, and `sret_arg=0`.
- Field-copy facts at offsets `0`, `4`, and `8` describe the aggregate value
  movement; RV64 should consume those facts rather than reconstruct the
  aggregate layout.
- Remaining select, local-publication, or pointer residuals exposed after
  aggregate lowering should be recorded as separate disposition, not silently
  folded into this source idea.

## Execution Rules

- Start with probes that isolate the current unsupported fragment for
  `20000917-1` and `20020206-1`; do not jump straight to broad lowering.
- Add focused backend tests around prepared aggregate `sret` memory-return
  facts before or with the first implementation change.
- Preserve fail-closed behavior when required prepared memory-return or
  frame-slot facts are missing.
- If implementation discovers a producer gap, stop and record the gap in
  `todo.md` for lifecycle disposition rather than bypassing it in RV64.
- For code/test packets, use:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Steps

### Step 1: Probe Coherent SRet Residual Shape

Re-run or inspect focused raw/prepared/object probes for `src/20000917-1.c` and
`src/20020206-1.c`. Identify the first RV64 unsupported fragment after
producer-clean aggregate `sret` facts, and choose the smallest testable packet.

Completion signal: `todo.md` names the exact unsupported fragment, relevant
prepared facts, and the first implementation/test packet.

### Step 2: Add Focused Prepared SRet Contract Coverage

Add or extend backend coverage for coherent prepared aggregate `sret`
memory-return facts. The test should exercise prepared facts, not testcase
names or inferred source layout.

Completion signal: focused coverage proves the expected frame-slot
memory-return and field-copy authority, and fails closed when required facts
are absent.

### Step 3: Lower Caller-Side Aggregate SRet Memory Return

Implement the bounded RV64 lowering needed to materialize caller-side aggregate
`sret` memory-return setup and post-call aggregate value publication from
prepared facts.

Completion signal: the focused backend proof passes and representative probes
show the aggregate `sret` unsupported fragment is gone or replaced by a smaller
correctly classified non-aggregate residual.

### Step 4: Residual Disposition And Close Readiness

Classify any remaining select, local-publication, or pointer residuals exposed
after aggregate `sret` lowering. Decide whether they belong to existing
follow-up ideas or require new durable source ideas.

Completion signal: lifecycle review can close this idea, or keep it active
with an exact remaining in-scope aggregate `sret` lowering packet.
