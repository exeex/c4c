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
  - Notes: The latest slice moved the folded two-argument direct-call helper contract behind `parse_backend_structured_folded_two_arg_function()` in `src/backend/lowering/call_decode.hpp`. `src/backend/aarch64/codegen/emit.cpp` now reuses that lowering-owned parser instead of keeping a target-local add/sub helper-body scan, and shared regression coverage proves the parser preserves renamed parameter names, helper SSA values, and the folded base immediate. Earlier Step 3 slices had already extracted the structured zero-argument return-immediate helper, the single-argument add-immediate helper, the structured two-parameter add helper, the legacy single-helper direct-call parser, the call-crossing source parser, and the two-argument local/direct-call LIR module-shape parsing behind lowering-owned decode.
  - Blockers: None.
- [ ] Step 4: Shift target codegen onto backend semantics
  - Notes: Defer until Step 3 lands enough structured operand and call surfaces.
  - Blockers: Depends on Step 3.
- [ ] Step 5: Validate positioning for later BIR work
  - Notes: Closeout only after the active structural slice lands and backend validation is stable.
  - Blockers: Depends on prior steps.

Current Step: Step 3
Next Step: Continue the Step 3 audit by checking whether any remaining structured direct-call helper/call contracts still duplicate lowering-owned decode in target emitters, or whether the next highest-value extraction is now one of the remaining handwritten-asm-only matcher seams outside the folded direct-call helper family.
Blockers: None

Latest Iteration Note: The Step 3 audit now covers the folded two-argument direct-call helper contract used by the AArch64 asm path. Shared lowering-owned parsing now exposes `parse_backend_structured_folded_two_arg_function()`, and the AArch64 matcher consumes that backend-owned seam instead of keeping a target-local helper-body scan for the folded add/sub helper.

Iteration Update: Added `parse_backend_structured_folded_two_arg_function()` to `src/backend/lowering/call_decode.hpp`, rewired `src/backend/aarch64/codegen/emit.cpp` to consume that shared backend helper-body parser for the folded two-argument direct-call slice, and added shared regression coverage in `tests/backend/backend_lir_adapter_tests.cpp` proving the seam preserves renamed parameter names, helper SSA values, and folded base immediates without target-local helper-body scans.

Validation: Baseline `test_fail_before.log` remained the existing `2667` passed / `2` failed reference point for this plan. `cmake -S . -B build` passed. `cmake --build build -j8 --target backend_lir_adapter_tests backend_lir_adapter_x86_64_tests backend_lir_adapter_aarch64_tests` passed. `ctest --test-dir build -R '^backend_lir_adapter_tests$' --output-on-failure`, `ctest --test-dir build -R '^backend_lir_adapter_x86_64_tests$' --output-on-failure`, and `ctest --test-dir build -R '^backend_lir_adapter_aarch64_tests$' --output-on-failure` all passed for this slice. Full rebuild `cmake --build build -j8` passed. Full-suite `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log` finished with the same `2667` passed / `2` failed summary, and `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed` passed with zero new failing tests. The only remaining failures are still the same two known AArch64 object-contract cases already recorded for this plan: `backend_contract_aarch64_return_add_object` and `backend_contract_aarch64_global_load_object`.

Next Slice Hint: Keep extracting shared backend-owned decode for any remaining structured direct-call seams before broadening Step 4 target-codegen work, then reassess whether the next Step 3 value is in target-only handwritten-asm matcher glue rather than helper-body parsing.
