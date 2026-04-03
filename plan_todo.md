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
  - Notes: The latest slice removed the duplicate AArch64-local legacy call-crossing regalloc-source parser from `src/backend/aarch64/codegen/emit.cpp` and rewired that fallback to reuse the shared lowering-owned `parse_backend_single_helper_call_crossing_source_value()` helper that x86 already consumes. Shared regression coverage in `tests/backend/backend_lir_adapter_tests.cpp` now also proves the helper preserves renamed helper parameter names plus renamed source, call, and return SSA values for the legacy call-crossing seam. The prior slice introduced `parse_backend_minimal_declared_direct_call_module()` in `src/backend/lowering/call_decode.hpp`, rewired the x86 declared-direct-call seam in `src/backend/x86/codegen/emit.cpp` to consume that lowering-owned backend-module matcher, and added shared regression coverage proving the parser preserves renamed declaration symbols, renamed parameter names, typed call operands, vararg signatures, and fixed return immediates without target-local backend-module scans. The earlier Step 3 slices introduced `parse_backend_minimal_folded_two_arg_direct_call_module()` and `parse_backend_minimal_structured_direct_call_module()` in `src/backend/lowering/call_decode.hpp`, moved the structured call-crossing backend-module contract behind `parse_backend_minimal_call_crossing_direct_call_module()`, and centralized the x86/AArch64 structured direct-call matcher family behind lowering-owned decode.
  - Blockers: None.
- [ ] Step 4: Shift target codegen onto backend semantics
  - Notes: Step 4 x86 cutovers continue to replace emitter-local compatibility reshaping with direct lowering-owned parsed-module views. The zero-argument direct-call asm path consumes `ParsedBackendMinimalDirectCallModuleView` directly, `direct_call_add_imm` now emits from `ParsedBackendMinimalDirectCallAddImmModuleView`, and the latest x86 slice removes the remaining backend-IR `two_arg_direct_call` reshaping layer so the x86 asm fast path emits directly from `ParsedBackendMinimalTwoArgDirectCallModuleView`. The newer AArch64 slices now mirror that pattern for the backend-owned structured direct-call family: `src/backend/aarch64/codegen/emit.cpp` first dropped the target-local `direct_call_add_imm` wrapper, and this iteration removes the remaining AArch64 backend-IR `two_arg_direct_call` reshaping layer so the asm fast path now dispatches and emits directly from `ParsedBackendMinimalTwoArgDirectCallModuleView`. The renamed structured AArch64 regression now also mutates helper parameter names, helper SSA names, main call SSA names, and both direct-call immediates while still holding the asm fast path.
  - Blockers: Keep additional emitter cutovers behind the remaining Step 3 structured-surface work.
- [ ] Step 5: Validate positioning for later BIR work
  - Notes: Closeout only after the active structural slice lands and backend validation is stable.
  - Blockers: Depends on prior steps.

Current Step: Step 4
Next Step: Inspect the remaining AArch64 structured direct-call variants, starting with the folded/local two-argument seams, for any leftover emitter-local compatibility reshaping that should be cut over to lowering-owned parsed-module metadata.
Blockers: None

Latest Iteration Note: The AArch64 baseline `two_arg_direct_call` Step 4 slice is now landed: the emitter no longer rebuilds a target-local compatibility slice and instead emits directly from `ParsedBackendMinimalTwoArgDirectCallModuleView`, while the renamed structured regression now also mutates helper parameter names, helper and call SSA names, and both lowered call immediates without falling off the asm path.

Iteration Update: Tightened `test_aarch64_backend_scaffold_accepts_renamed_structured_two_arg_direct_call_ir_without_signature_shims()` so the renamed lowered helper body must still render as the direct asm fast path after helper-parameter renames, helper/main SSA renames, and direct-call immediate changes with legacy signature/call-type shims cleared, then removed the AArch64-local lowered-IR `two_arg_direct_call` reshaping layer by emitting from `c4c::backend::ParsedBackendMinimalTwoArgDirectCallModuleView` directly in `src/backend/aarch64/codegen/emit.cpp`.

Validation: Baseline `test_fail_before.log` remained the existing `2667` passed / `2` failed reference point for this plan. `cmake --build build -j8 --target backend_lir_adapter_aarch64_tests backend_lir_adapter_tests backend_ir_tests` passed. `ctest --test-dir build -R '^backend_lir_adapter_aarch64_tests$' --output-on-failure` passed, and `ctest --test-dir build -R '^(backend_lir_adapter_tests|backend_ir_tests)$' --output-on-failure` passed for the nearby shared backend surfaces. Full-suite `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log` finished with the same `2667` passed / `2` failed summary, and `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed` passed with zero new failing tests. The only remaining failures are still the same two known AArch64 object-contract cases already recorded for this plan: `backend_contract_aarch64_return_add_object` and `backend_contract_aarch64_global_load_object`.

Next Slice Hint: Inspect the remaining AArch64 folded/local two-argument direct-call seams for the same backend-owned parsed-module cutover pattern now used by the baseline `two_arg_direct_call` and `direct_call_add_imm` asm paths.

Iteration Target: Completed Step 4 AArch64 baseline `two_arg_direct_call` cutover. Next iteration should target the next AArch64 structured direct-call seam that still needs the same direct parsed-module pattern.
