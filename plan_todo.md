Status: Active
Source Idea: ideas/open/04_backend_ir_to_bir_scaffold.md
Source Plan: plan.md

# Active Queue: Backend IR To BIR Scaffold

## Queue
- [x] Step 1: Establish the BIR type scaffold
  - Notes: Added explicit `src/backend/bir*` scaffold files with a minimal module/function/block/return/value model, tiny printer/validator helpers, and a separate `backend_bir_tests` executable plus CTest registration.
  - Blockers: None.
- [ ] Step 2: Add a flag-gated LIR-to-BIR entrypoint
  - Notes: The new flow must stay opt-in and leave the existing backend path as default.
  - Blockers: None.
- [ ] Step 3: Prove one end-to-end BIR backend case
  - Notes: Use a new BIR-oriented test surface rather than growing the legacy adapter suites into a mixed framework.
  - Blockers: None.
- [ ] Step 4: Validate scaffold stability
  - Notes: Keep regression scope backend-focused and record bounded deferrals for the next BIR-enablement idea.
  - Blockers: None.

## Current Focus

Step 2: add a flag-gated LIR-to-BIR entrypoint without disturbing the default backend flow

## Immediate Next Slice

Inspect the backend option/entrypoint seam and thread through the narrowest opt-in switch that can select a future BIR lowering path while preserving the current legacy backend path as the default.

## Accomplished Context

- `03_lir_to_backend_ir_refactor.md` has been closed and archived with a current validation snapshot.
- The cleaned-up lowering boundary now exists under `src/backend/lowering/lir_to_backend_ir.*`.
- The next open idea in numeric order is `04_backend_ir_to_bir_scaffold.md`.
- Step 1 landed as a source-only BIR scaffold slice: explicit `bir` naming, tiny validation/printing helpers, and a separate BIR-oriented test executable.
- Validation after the scaffold slice: `cmake --build build -j8` succeeded; `./build/backend_bir_tests` passed; `ctest --test-dir build -R backend_bir_tests --output-on-failure` passed; `ctest --test-dir build -R 'backend_' --output-on-failure > test_fail_before.log` and `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log` both ended with the same two known failures (`backend_contract_aarch64_return_add_object`, `backend_contract_aarch64_global_load_object`); the regression-guard script passed with zero new failing tests.

## Risks And Constraints

- Do not cut over the default backend path in this plan.
- Do not broaden the initial BIR model beyond what one tiny end-to-end case requires.
- Do not fold new-path tests into legacy adapter-heavy files unless unavoidable.
- Keep transitional `backend_ir` compatibility seams intact unless a tiny wrapper change is enough.

## Validation Baseline

- Current full-suite snapshot: `2667 passed / 2 failed / 2669 total`.
- Known remaining failures: `backend_contract_aarch64_return_add_object` and `backend_contract_aarch64_global_load_object`.
- Backend-scoped regression work for this plan should preserve the default-path ceiling while introducing opt-in BIR coverage.
