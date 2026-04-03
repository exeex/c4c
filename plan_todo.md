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
  - Notes: The latest slice introduced `parse_backend_minimal_structured_direct_call_module()` in `src/backend/lowering/call_decode.hpp` as the shared backend-module matcher for the structured direct-call family, rewired the AArch64 zero-argument, single-add-immediate, two-argument add, and folded-two-argument direct-call seams to consume that lowering-owned matcher, and added shared regression coverage proving the matcher preserves renamed helper symbols, renamed helper parameter names, and direct-call operands without target-local backend-module scans. The prior slice had moved the structured call-crossing backend-module contract behind `parse_backend_minimal_call_crossing_direct_call_module()` in `src/backend/lowering/call_decode.hpp`, and both `src/backend/x86/codegen/emit.cpp` and `src/backend/aarch64/codegen/emit.cpp` now reuse that lowering-owned parser instead of re-deriving the main-block shape inline. Shared regression coverage already proves the call-crossing parser preserves renamed helper symbols, renamed source/call/final-add SSA values, and summed source immediates without target-local backend-module scans. Earlier Step 3 slices had already moved the x86 backend-module zero-argument, single-argument add-immediate, and two-argument direct-call contracts behind `parse_backend_minimal_direct_call_module()`, `parse_backend_minimal_direct_call_add_imm_module()`, and `parse_backend_minimal_two_arg_direct_call_module()`. Earlier Step 3 slices had also extracted the structured zero-argument return-immediate helper, the single-argument add-immediate helper, the structured two-parameter add helper, the folded two-argument helper contract, the legacy single-helper direct-call parser, the call-crossing source parser, and the two-argument local/direct-call LIR module-shape parsing behind lowering-owned decode.
  - Blockers: None.
- [ ] Step 4: Shift target codegen onto backend semantics
  - Notes: Defer until Step 3 lands enough structured operand and call surfaces.
  - Blockers: Depends on Step 3.
- [ ] Step 5: Validate positioning for later BIR work
  - Notes: Closeout only after the active structural slice lands and backend validation is stable.
  - Blockers: Depends on prior steps.

Current Step: Step 3
Next Step: Reassess the remaining target-local structured direct-call helper-body matchers and decide whether the next Step 3 slice should centralize the zero-arg/add-imm/two-arg helper-shape discovery now that backend-module matching lives in lowering, or pivot to another handwritten-asm matcher that still duplicates backend-owned shape checks.
Blockers: None

Latest Iteration Note: Shared lowering-owned parsing now also covers the structured direct-call-family backend-module matcher itself. AArch64 consumes the new `parse_backend_minimal_structured_direct_call_module()` seam for the zero-arg, add-immediate, two-arg add, and folded-two-arg cases, so the remaining direct-call-family duplication is now narrower and concentrated in the target-local helper-body matchers that sit below these module-level parsers.

Iteration Update: Added `parse_backend_minimal_structured_direct_call_module()` to `src/backend/lowering/call_decode.hpp`, rewired the AArch64 structured direct-call-family module parsing paths to consume that shared backend-module matcher, refactored the existing zero-arg/add-imm/two-arg lowering parsers to build on the same matcher, and added shared regression coverage in `tests/backend/backend_lir_adapter_tests.cpp` proving the seam preserves renamed helper symbols, renamed parameter names, and direct-call operands without target-local backend-module scans.

Validation: Baseline `test_fail_before.log` remained the existing `2667` passed / `2` failed reference point for this plan. `cmake --build build -j8 --target backend_lir_adapter_tests backend_lir_adapter_x86_64_tests backend_lir_adapter_aarch64_tests` passed. `ctest --test-dir build -R '^backend_lir_adapter_tests$' --output-on-failure`, `ctest --test-dir build -R '^backend_lir_adapter_x86_64_tests$' --output-on-failure`, and `ctest --test-dir build -R '^backend_lir_adapter_aarch64_tests$' --output-on-failure` all passed for this slice. Full-suite `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log` finished with the same `2667` passed / `2` failed summary, and `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed` passed with zero new failing tests. The only remaining failures are still the same two known AArch64 object-contract cases already recorded for this plan: `backend_contract_aarch64_return_add_object` and `backend_contract_aarch64_global_load_object`.

Next Slice Hint: Audit the remaining target-local `match_minimal_structured_direct_call*_helper` family in `src/backend/aarch64/codegen/emit.cpp` and the corresponding structured helper-body matchers elsewhere, then decide whether the next Step 3 slice should centralize those helper-shape parsers under lowering-owned decode, or instead move to a different handwritten-asm matcher that still duplicates structured backend IR shape checks.
