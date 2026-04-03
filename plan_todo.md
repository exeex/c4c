Status: Active
Source Idea: ideas/open/05_bir_enablement_and_test_harness_refactor.md
Source Plan: plan.md

# Active Queue: BIR Enablement And Backend Test Harness Refactor

## Queue
- [ ] Step 1: Establish the first layered BIR test harness split
  - Notes: Start by separating BIR lowering / validation / pipeline concerns before adding more behavior coverage.
  - Blockers: None.
- [ ] Step 2: Expand BIR lowering for one additional behavior cluster
  - Notes: Pick one bounded cluster beyond the scaffold return-add path and keep it flag-gated.
  - Blockers: None.
- [ ] Step 3: Add layered BIR assertions for the new cluster
  - Notes: Prefer direct BIR-shape checks before end-to-end emission assertions.
  - Blockers: None.
- [ ] Step 4: Validate backend-scoped regression stability
  - Notes: Preserve the current known-failure ceiling while extending the BIR path.
  - Blockers: None.

## Current Focus

Step 1: carve the first reusable BIR-oriented test-harness split out of the initial scaffold work

## Immediate Next Slice

Inspect the existing `tests/backend/backend_bir_tests.cpp` coverage and decide the smallest helper/file split that cleanly separates lowering/validation assertions from route-smoke assertions for the next BIR behavior cluster.

## Accomplished Context

- `04_backend_ir_to_bir_scaffold.md` is closed and archived.
- The repo now has explicit `lir_to_bir` and `bir_to_backend_ir` seams plus an internal `BackendPipeline::Bir` option.
- The initial BIR scaffold covers the tiny single-block i32 return/add path and reaches x86 emission end-to-end.
- Validation at close of the scaffold plan: `./build/backend_bir_tests` passed; `ctest --test-dir build -R 'backend_' --output-on-failure` ended with only the two known AArch64 contract failures; `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log` ended with the same two known failures; the regression guard reported zero new failing tests.

## Risks And Constraints

- Do not cut over the default backend path.
- Do not let the new BIR test layout sprawl into a large rewrite before it proves value.
- Keep new-path coverage and legacy-path coverage conceptually separate.
- Treat wider BIR instruction coverage as incremental slices, not one broad port.

## Validation Baseline

- Current full-suite snapshot: `2668 passed / 2 failed / 2670 total`.
- Known remaining failures: `backend_contract_aarch64_return_add_object` and `backend_contract_aarch64_global_load_object`.
- Backend-focused validation for this plan should preserve that ceiling while new BIR coverage grows behind the flag.
