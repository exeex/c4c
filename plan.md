# Prepared Aggregate ABI Contract Review Plan

Status: Active
Source Idea: ideas/open/431_prepared_aggregate_abi_contract_review.md

## Purpose

Separate coherent aggregate `sret`/`byval` call-storage rows from prepared ABI
producer defects before any RV64 aggregate call-storage lowering. The source
idea is complete when the aggregate ABI bucket has a durable disposition:
coherent prepared facts, producer defects, and any later RV64 aggregate lowering
scope are separated without broadening this review into target implementation.

## Core Rule

Prepared ABI facts are the authority. Do not infer aggregate size, alignment,
storage class, return slot, byval copy, or call-home ownership in RV64 from
source, raw BIR shape, or testcase identity.

## Read First

- `ideas/open/431_prepared_aggregate_abi_contract_review.md`
- `docs/rv64_gcc_torture_post_contract/failure_bucket_map.md`
- `docs/rv64_gcc_torture_post_contract/followup_idea_plan.md`
- `src/backend/bir/lir_to_bir/calling.cpp`
- `src/backend/bir/lir_to_bir/call_abi.cpp`
- `src/backend/bir/bir.hpp`
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/regalloc/call_return_abi.cpp`
- `src/backend/prealloc/prepared_contract_verifier.cpp`
- `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`
- `tests/backend/bir/backend_prealloc_call_boundary_classification_test.cpp`
- `tests/backend/bir/backend_prealloc_prepared_contract_verifier_test.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`

## Current Targets

- Aggregate `sret`/`byval` call-storage rows from the post-contract RV64
  gcc-torture bucket.
- Size, alignment, storage class, and call-home ownership facts.
- Representative rows: `src/pr88904.c`, `src/20000917-1.c`,
  `src/20010224-1.c`, `src/20020206-1.c`, `src/20020506-1.c`.

## Non-Goals

- Implementing RV64 aggregate call-storage lowering in this review plan.
- Treating aggregate rows as scalar call result or argument publication.
- F128, long-double, or runtime-helper policy.
- Reconstructing aggregate ABI facts in RV64 from source or raw BIR.
- Expectation, allowlist, or pass/fail accounting changes.

## Working Model

- BIR carries aggregate ABI attributes such as `sret`/`byval` and size/alignment
  metadata.
- Prepared call and ABI planning must expose coherent aggregate storage, copy,
  and return-slot facts before target lowering consumes them.
- Suspicious size/alignment or ownership facts are producer defects, not target
  lowering TODOs.
- Coherent rows may justify a later RV64 aggregate lowering idea, but that work
  is outside this review source idea.

## Execution Rules

- Step 1 may be classification-only and should not edit implementation files.
- Add focused fail-closed contract tests only if suspicious prepared facts are
  confirmed and testable within this source idea.
- Do not broaden this runbook into RV64 aggregate call lowering.
- If coherent RV64 aggregate lowering scope appears, record follow-up source
  idea details instead of implementing the lowering here.
- For docs/classification-only work, use `git diff --check` as the local proof.
- For test edits, use:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Steps

### Step 1: Audit Aggregate ABI Evidence And Choose First Review Packet

Inspect the representative aggregate `sret`/`byval` rows and post-contract
handoff evidence. Classify each row by ABI form, size/alignment coherence,
storage class, call-home ownership, and suspicious producer facts. Select the
first bounded review packet and record the exact evidence and proof command in
`todo.md`.

Completion signal: `todo.md` names the first executable review target, the
evidence artifacts to inspect or probe, the expected classification output, and
the proof command.

### Step 2: Classify Prepared ABI Fact Coherence

Inspect BIR and prepared ABI facts for the selected rows. Separate coherent
prepared aggregate ABI facts from producer defects. Add focused fail-closed
contract tests only when suspicious facts are confirmed and local contracts can
express the failure.

Completion signal: classification is recorded in `todo.md`, confirmed producer
defects have a repair route or fail-closed contract coverage, and any touched
tests pass.

### Step 3: Produce Follow-Up Disposition

Record separate follow-up disposition for producer repairs and for any coherent
RV64 aggregate lowering scope. Keep target lowering out of this review plan.

Completion signal: `todo.md` contains source-idea-ready follow-up notes, with
rows grouped by coherent lowering scope versus producer/prepared ABI defects.

### Step 4: Close-Readiness Review

Validate whether the source idea has enough classification and proof to close,
or whether lifecycle split/repair is required.

Completion signal: lifecycle review can close, split, or keep the idea active
with an exact remaining in-scope prepared ABI review packet.
