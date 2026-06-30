# PR88904 SRet Object Home Producer Review Plan

Status: Active
Source Idea: ideas/open/436_pr88904_sret_object_home_producer_review.md

## Purpose

Reconcile `src/pr88904.c` aggregate `sret` size, alignment, and object-home
facts before the row can be admitted to any RV64 aggregate lowering scope.

## Goal

Decide whether the `pr88904` `sret_param` object record and call memory-return
record have a coherent prepared-contract interpretation, or whether they expose
a producer defect that needs repair or fail-closed coverage.

## Core Rule

Prepared producer facts are the authority. Do not consume `pr88904` as RV64
aggregate lowering evidence until size, alignment, and object-home facts are
reconciled by producer-owned review.

## Read First

- `ideas/open/436_pr88904_sret_object_home_producer_review.md`
- `ideas/closed/431_prepared_aggregate_abi_contract_review.md`
- `build/agent_state/431_step1_aggregate_abi_audit/evidence.md`
- `build/agent_state/431_step1_aggregate_abi_audit/classification.tsv`
- `build/agent_state/431_step2_abi_fact_coherence/classification.md`
- `build/agent_state/431_step1_aggregate_abi_audit/pr88904.bir.out`
- `build/agent_state/431_step1_aggregate_abi_audit/pr88904.prepared.out`
- `build/agent_state/431_step1_aggregate_abi_audit/pr88904.object.err`
- `src/backend/bir/lir_to_bir/calling.cpp`
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/prepared_contract_verifier.cpp`
- `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`
- `tests/backend/bir/backend_prealloc_prepared_contract_verifier_test.cpp`

## Current Targets

- `src/pr88904.c` real aggregate `sret(size=8, align=4)` call facts.
- Prepared object metadata reporting `sret_param` object `size=8 align=8`.
- Prepared call memory record reporting `memory_size=8 memory_align=4`.
- Object-home ownership for the `sret` frame-slot path.

## Non-Goals

- Implementing RV64 aggregate lowering for `pr88904`.
- Fixing `pr88904` inline-asm, clobber, store-publication, select, or
  local-publication residuals.
- Folding `pr88904` into the coherent `20000917-1` / `20020206-1` lowering
  scope before producer facts are reconciled.
- Inferring object layout, alignment, or home ownership in RV64 from source or
  testcase identity.
- Expectation, allowlist, unsupported-marker, runtime-comparison, or pass/fail
  accounting changes.

## Working Model

- `pr88904` has real aggregate `sret` facts, unlike the scalar metadata defect
  rows closed by idea 434.
- The observed alignment mismatch may be coherent if object alignment and call
  ABI memory alignment intentionally describe different contracts.
- If the mismatch is accidental or ambiguous, the producer contract must state
  that explicitly through repair, verifier coverage, or durable follow-up
  disposition.

## Execution Rules

- Start with classification and focused evidence; do not implement target
  lowering in this plan.
- Add focused producer contract tests only when the accepted or rejected
  interpretation is clear enough to encode.
- Keep unrelated `pr88904` residuals separate from the `sret` object-home
  review.
- If no implementation is needed, close with docs/classification proof and
  durable follow-up disposition.
- For code/test packets, use:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Steps

### Step 1: Audit PR88904 Prepared SRet Facts

Inspect `pr88904` raw BIR, prepared BIR, and object-route evidence. Classify
the `sret_param` object size/alignment, call memory size/alignment, frame-slot
home, and unrelated residuals.

Completion signal: `todo.md` records whether the mismatch is likely coherent,
defective, or ambiguous, and names the first bounded producer-contract packet.

### Step 2: Add Or Confirm Producer Contract Coverage

Add focused producer/verifier coverage if the mismatch is a defect or accepted
contract that needs a guard. If existing coverage already proves the contract,
record the exact proof instead.

Completion signal: focused proof covers the accepted or rejected
size/alignment/object-home interpretation.

### Step 3: Produce Follow-Up Eligibility Disposition

Decide whether `pr88904` can later join coherent RV64 aggregate `sret` lowering
or must remain parked behind a producer repair. Route inline-asm,
store-publication, select, and local-publication residuals separately.

Completion signal: `todo.md` contains a durable follow-up disposition for
`pr88904` eligibility and unrelated residuals.

### Step 4: Close-Readiness Review

Validate whether the source idea is complete or still has an in-scope
producer-contract packet.

Completion signal: lifecycle review can close this idea or keep it active with
an exact remaining producer review task.
