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
  - Notes: The latest slice moved the structured zero-argument return-immediate and single-argument add-immediate helper contracts behind `parse_backend_structured_zero_arg_return_imm_function()` and `parse_backend_structured_single_add_imm_function()` in `src/backend/lowering/call_decode.hpp`. Both `src/backend/x86/codegen/emit.cpp` and `src/backend/aarch64/codegen/emit.cpp` now reuse those backend-owned helper-body parsers instead of keeping duplicate target-local callee-body scans. Shared regression coverage now proves the parsers preserve renamed structured helper symbols, parameter names, and helper SSA values. Earlier Step 3 slices had already extracted the structured two-parameter add helper, the legacy single-helper direct-call parser, the call-crossing source parser, and the two-argument local/direct-call LIR module-shape parsing behind lowering-owned decode.
  - Blockers: None.
- [ ] Step 4: Shift target codegen onto backend semantics
  - Notes: Defer until Step 3 lands enough structured operand and call surfaces.
  - Blockers: Depends on Step 3.
- [ ] Step 5: Validate positioning for later BIR work
  - Notes: Closeout only after the active structural slice lands and backend validation is stable.
  - Blockers: Depends on prior steps.

Current Step: Step 3
Next Step: Continue the Step 3 audit by checking whether any remaining structured direct-call helper/call contracts still duplicate lowering-owned decode in target emitters, or whether the next highest-value extraction is one of the remaining handwritten-asm-only matcher seams.
Blockers: None

Latest Iteration Note: The Step 3 audit now covers the structured zero-argument return-immediate and single-argument add-immediate helper contracts used by the direct-call asm paths. Shared lowering-owned parsing now exposes `parse_backend_structured_zero_arg_return_imm_function()` and `parse_backend_structured_single_add_imm_function()`, and the x86 plus AArch64 matchers consume those backend-owned seams instead of keeping duplicate target-local helper-body scans.

Iteration Update: Added `parse_backend_structured_zero_arg_return_imm_function()` and `parse_backend_structured_single_add_imm_function()` to `src/backend/lowering/call_decode.hpp`, rewired `src/backend/x86/codegen/emit.cpp` and `src/backend/aarch64/codegen/emit.cpp` to consume those shared backend helper-body parsers for structured zero-argument and single-argument direct calls, and added shared regression coverage in `tests/backend/backend_lir_adapter_tests.cpp` proving the seams preserve renamed structured helper symbols, parameter names, and helper SSA values without target-local helper-body scans.

Validation: Baseline `test_fail_before.log` remained the existing `2667` passed / `2` failed reference point for this plan. `cmake -S . -B build` passed. `cmake --build build -j8 --target backend_lir_adapter_tests backend_lir_adapter_x86_64_tests backend_lir_adapter_aarch64_tests` passed. `ctest --test-dir build -R '^backend_lir_adapter_tests$' --output-on-failure`, `ctest --test-dir build -R '^backend_lir_adapter_x86_64_tests$' --output-on-failure`, and `ctest --test-dir build -R '^backend_lir_adapter_aarch64_tests$' --output-on-failure` all passed for this slice. Full rebuild `cmake --build build -j8` passed. Full-suite `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log` finished with the same `2667` passed / `2` failed summary, and `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed` passed with zero new failing tests. The only remaining failures are still the same two known AArch64 object-contract cases already recorded for this plan: `backend_contract_aarch64_return_add_object` and `backend_contract_aarch64_global_load_object`.

Next Slice Hint: Keep extracting shared backend-owned decode for structured direct-call seams before broadening Step 4 target-codegen work.
