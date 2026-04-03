Status: Active
Source Idea: ideas/open/03_lir_to_backend_ir_refactor.md
Source Plan: plan.md

# Active Queue: LIR To Backend IR Refactor

## Queue
- [x] Step 1: Re-establish the lowering boundary
  - Notes: Production lowering moved behind `src/backend/lowering/lir_to_backend_ir.*`, with `src/backend/lir_adapter.hpp` kept as the compatibility shim.
  - Blockers: None.
- [x] Step 2: Isolate LIR syntax decoding
  - Notes: Shared decode/parsing helpers in `src/backend/lowering/call_decode.*` now own the staged LIR call and signature parsing that had been spread across target emitters.
  - Blockers: None.
- [ ] Step 3: Make backend IR more backend-native
  - Notes: The latest slice introduced `parse_backend_minimal_declared_direct_call_module()` in `src/backend/lowering/call_decode.hpp`, rewired the x86 declared-direct-call seam in `src/backend/x86/codegen/emit.cpp` to consume that lowering-owned backend-module matcher, and added shared regression coverage proving the parser preserves renamed declaration symbols, renamed parameter names, typed call operands, vararg signatures, and fixed return immediates without target-local backend-module scans. The prior slice had introduced `parse_backend_minimal_folded_two_arg_direct_call_module()` in `src/backend/lowering/call_decode.hpp`, rewired the AArch64 zero-argument, single-add-immediate, two-argument add, and folded-two-argument direct-call seams to consume the shared lowering-owned module parsers end to end, and added shared regression coverage proving the folded parser preserves renamed helper symbols, renamed helper parameter names, direct-call operands, and folded return immediates without target-local helper-body scans. The earlier Step 3 slices had introduced `parse_backend_minimal_structured_direct_call_module()` in `src/backend/lowering/call_decode.hpp` as the shared backend-module matcher for the structured direct-call family, moved the structured call-crossing backend-module contract behind `parse_backend_minimal_call_crossing_direct_call_module()`, and centralized the x86 backend-module zero-argument, single-argument add-immediate, and two-argument direct-call contracts behind lowering-owned decode.
  - Blockers: None.
- [ ] Step 4: Shift target codegen onto backend semantics
  - Notes: Defer until Step 3 lands enough structured operand and call surfaces.
  - Blockers: Depends on Step 3.
- [ ] Step 5: Validate positioning for later BIR work
  - Notes: Closeout only after the active structural slice lands and backend validation is stable.
  - Blockers: Depends on prior steps.

Current Step: Step 3
Next Step: Audit the remaining Step 3 backend-module matcher seams in `src/backend/x86/codegen/emit.cpp` and `src/backend/aarch64/emit.cpp` for any helper-shape contracts that still sit outside `src/backend/lowering/call_decode.hpp`; if no comparable direct-call seam remains, start positioning the first Step 4 target-emitter cutover slice.
Blockers: None

Latest Iteration Note: The x86 structured single-add-immediate direct-call scaffold path now proves it only depends on shared lowering-owned metadata: the renamed regression mutates helper parameter names, helper SSA names, call SSA names, and the direct-call immediate, and validation stayed green after removing the leftover x86-local structured direct-call matcher scaffolding.

Iteration Update: Strengthened the x86 renamed structured single-add-immediate scaffold regression in `tests/backend/backend_lir_adapter_x86_64_tests.cpp` so it now renames the helper parameter, helper SSA result, call result, and direct-call immediate alongside the callee symbol, then removed the orphaned x86-local structured direct-call matcher and add-immediate helper matcher from `src/backend/x86/codegen/emit.cpp` because the active scaffold path already relies on the shared lowering-owned parser entrypoints.

Validation: Baseline `test_fail_before.log` remained the existing `2667` passed / `2` failed reference point for this plan. `cmake --build build -j8 --target backend_lir_adapter_tests backend_lir_adapter_x86_64_tests` passed. `ctest --test-dir build -R '^backend_lir_adapter_tests$' --output-on-failure` and `ctest --test-dir build -R '^backend_lir_adapter_x86_64_tests$' --output-on-failure` passed for this slice. Full-suite `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log` finished with the same `2667` passed / `2` failed summary, and `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed` passed with zero new failing tests. The only remaining failures are still the same two known AArch64 object-contract cases already recorded for this plan: `backend_contract_aarch64_return_add_object` and `backend_contract_aarch64_global_load_object`.

Next Slice Hint: Audit the remaining x86 handwritten-asm matcher helpers for places that still re-check structured backend helper shapes locally even though lowering already owns comparable contracts, then centralize the narrowest remaining seam with shared regression coverage.
