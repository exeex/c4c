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
  - Notes: Step 4 x86 cutovers continue to replace emitter-local compatibility reshaping with direct lowering-owned parsed-module views. The zero-argument direct-call asm path consumes `ParsedBackendMinimalDirectCallModuleView` directly, `direct_call_add_imm` now emits from `ParsedBackendMinimalDirectCallAddImmModuleView`, and the latest x86 slice removes the remaining backend-IR `two_arg_direct_call` reshaping layer so the x86 asm fast path emits directly from `ParsedBackendMinimalTwoArgDirectCallModuleView`. The newest AArch64 slice now removes the target-local `direct_call_add_imm` wrapper too: `src/backend/aarch64/codegen/emit.cpp` dispatches and emits directly from `ParsedBackendMinimalDirectCallAddImmModuleView`, and the renamed structured AArch64 regression now mutates helper parameter names plus helper/call SSA values and the call immediate while still holding the asm fast path.
  - Blockers: Keep additional emitter cutovers behind the remaining Step 3 structured-surface work.
- [ ] Step 5: Validate positioning for later BIR work
  - Notes: Closeout only after the active structural slice lands and backend validation is stable.
  - Blockers: Depends on prior steps.

Current Step: Step 4
Next Step: Inspect the AArch64 `two_arg_direct_call` fast path for the same remaining target-local compatibility wrapper, then cut that seam over to emit directly from `ParsedBackendMinimalTwoArgDirectCallModuleView` with a tightened renamed structured regression.
Blockers: None

Latest Iteration Note: The AArch64 `direct_call_add_imm` Step 4 slice is now landed: the emitter no longer rebuilds a target-local compatibility slice and instead emits directly from `ParsedBackendMinimalDirectCallAddImmModuleView`, while the renamed structured regression now also mutates helper parameter names, helper and call SSA names, and the lowered call immediate without falling off the asm path.

Iteration Update: Tightened `test_x86_backend_scaffold_accepts_renamed_structured_two_arg_direct_call_ir_without_signature_shims()` so the renamed lowered helper body must still render as the direct asm fast path after helper-parameter and SSA renames with legacy signature/call-type shims cleared, then removed the x86-local lowered-IR `two_arg_direct_call` reshaping layer by emitting from `c4c::backend::ParsedBackendMinimalTwoArgDirectCallModuleView` directly in `src/backend/x86/codegen/emit.cpp`.

Validation: Baseline `test_fail_before.log` remained the existing `2667` passed / `2` failed reference point for this plan. `cmake --build build -j8 --target backend_lir_adapter_aarch64_tests backend_lir_adapter_tests backend_ir_tests` passed. `ctest --test-dir build -R '^backend_lir_adapter_aarch64_tests$' --output-on-failure` passed, and `ctest --test-dir build -R '^(backend_lir_adapter_tests|backend_ir_tests)$' --output-on-failure` passed for the nearby shared backend surfaces. Full-suite `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log` finished with the same `2667` passed / `2` failed summary, and `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed` passed with zero new failing tests. The only remaining failures are still the same two known AArch64 object-contract cases already recorded for this plan: `backend_contract_aarch64_return_add_object` and `backend_contract_aarch64_global_load_object`.

Next Slice Hint: Inspect the remaining AArch64 `two_arg_direct_call` asm seam for the same backend-IR reshaping wrapper pattern now removed from x86 and `direct_call_add_imm`, then cut it over to consume lowering-owned parsed-module metadata directly.

Iteration Target: Completed Step 4 AArch64 `direct_call_add_imm` cutover. Next iteration should target the AArch64 `two_arg_direct_call` seam using the same direct parsed-module pattern.
