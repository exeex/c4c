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
  - Notes: The latest slice moved the x86 backend-module zero-argument and single-argument add-immediate direct-call contracts behind `parse_backend_minimal_direct_call_module()` and `parse_backend_minimal_direct_call_add_imm_module()` in `src/backend/lowering/call_decode.hpp`. `src/backend/x86/codegen/emit.cpp` now reuses those lowering-owned parsers instead of keeping target-local backend-module scans for those structured direct-call slices, and shared regression coverage proves the parsers preserve renamed helper symbols, structured helper metadata, and direct-call immediates. The prior slice had already moved the x86 backend-module two-argument direct-call contract behind `parse_backend_minimal_two_arg_direct_call_module()`. Earlier Step 3 slices had already extracted the structured zero-argument return-immediate helper, the single-argument add-immediate helper, the structured two-parameter add helper, the folded two-argument helper contract, the legacy single-helper direct-call parser, the call-crossing source parser, and the two-argument local/direct-call LIR module-shape parsing behind lowering-owned decode.
  - Blockers: None.
- [ ] Step 4: Shift target codegen onto backend semantics
  - Notes: Defer until Step 3 lands enough structured operand and call surfaces.
  - Blockers: Depends on Step 3.
- [ ] Step 5: Validate positioning for later BIR work
  - Notes: Closeout only after the active structural slice lands and backend validation is stable.
  - Blockers: Depends on prior steps.

Current Step: Step 3
Next Step: Reassess the remaining backend-module call-crossing direct-call seam and decide whether it should move behind a single lowering-owned parser next, or whether the next highest-value Step 3 extraction has shifted to a different handwritten-asm matcher that still duplicates backend-module shape checks.
Blockers: None

Latest Iteration Note: Shared lowering-owned parsing now covers the backend-module zero-argument, single-argument add-immediate, and two-argument direct-call x86 slices. The remaining direct-call-family backend-module duplication is concentrated in the call-crossing seam, which now stands out as the next candidate for a shared parser if its mixed main-block arithmetic and regalloc handoff can be expressed cleanly.

Iteration Update: Added `parse_backend_minimal_direct_call_module()` and `parse_backend_minimal_direct_call_add_imm_module()` to `src/backend/lowering/call_decode.hpp`, rewired `src/backend/x86/codegen/emit.cpp` to consume those shared backend-module parsers for the structured zero-argument and single-argument add-immediate direct-call slices, removed the now-redundant x86-only zero-argument helper matcher, and added shared regression coverage in `tests/backend/backend_lir_adapter_tests.cpp` proving both new seams preserve renamed helper symbols, helper SSA values, and direct-call immediates without target-local backend-module scans.

Validation: Baseline `test_fail_before.log` remained the existing `2667` passed / `2` failed reference point for this plan. `cmake --build build -j8 --target backend_lir_adapter_tests backend_lir_adapter_x86_64_tests` passed. `ctest --test-dir build -R '^backend_lir_adapter_tests$' --output-on-failure` and `ctest --test-dir build -R '^backend_lir_adapter_x86_64_tests$' --output-on-failure` both passed for this slice. Full-suite `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log` finished with the same `2667` passed / `2` failed summary, and `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed` passed with zero new failing tests. The only remaining failures are still the same two known AArch64 object-contract cases already recorded for this plan: `backend_contract_aarch64_return_add_object` and `backend_contract_aarch64_global_load_object`.

Next Slice Hint: Audit the call-crossing direct-call backend-module matcher in `src/backend/x86/codegen/emit.cpp` and decide whether its mixed arithmetic-plus-call contract can be expressed as one lowering-owned parser without leaking x86 regalloc concerns; if not, pivot the next Step 3 slice toward a different handwritten-asm matcher that still duplicates structured backend-module checks.
