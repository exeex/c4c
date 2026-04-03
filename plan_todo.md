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
  - Notes: Step 4 x86 cutovers continue to replace emitter-local compatibility reshaping with direct lowering-owned parsed-module views. The zero-argument direct-call asm path consumes `ParsedBackendMinimalDirectCallModuleView` directly, `direct_call_add_imm` now emits from `ParsedBackendMinimalDirectCallAddImmModuleView`, and the latest x86 slice removes the remaining backend-IR `two_arg_direct_call` reshaping layer so the x86 asm fast path emits directly from `ParsedBackendMinimalTwoArgDirectCallModuleView`. The newer AArch64 slices now mirror that pattern for the backend-owned structured direct-call family: `src/backend/aarch64/codegen/emit.cpp` first dropped the target-local `direct_call_add_imm` wrapper, then removed the backend-IR `two_arg_direct_call` reshaping layer, then removed the folded two-argument compatibility wrapper, then removed the zero-argument direct-call compatibility wrapper, and this iteration removes the remaining AArch64-local `MinimalCallCrossingDirectCallSlice` reshaping layer so the call-crossing asm fast path now emits directly from `ParsedBackendMinimalCallCrossingDirectCallModuleView` while keying shared regalloc from lowering-owned source metadata.
  - Blockers: Keep additional emitter cutovers behind the remaining Step 3 structured-surface work.
- [ ] Step 5: Validate positioning for later BIR work
  - Notes: Closeout only after the active structural slice lands and backend validation is stable.
  - Blockers: Depends on prior steps.

Current Step: Step 3
Next Step: Re-scan the remaining backend IR surfaces for the next high-value structured direct-call or addressing seam that still forces parser-style helpers before additional Step 4 emitter cutovers.
Blockers: None

Latest Iteration Note: The AArch64 call-crossing Step 4 slice is now landed: the emitter no longer reshapes that path through `MinimalCallCrossingDirectCallSlice` and instead emits directly from `ParsedBackendMinimalCallCrossingDirectCallModuleView`, while the existing structured call-crossing regressions still hold on both the lowered backend-only path and the legacy-assisted regalloc path after helper-symbol and source-value renames.

Iteration Update: Removed the AArch64 call-crossing lowered-IR compatibility wrapper by deleting `MinimalCallCrossingDirectCallSlice`, wiring both backend-module entrypoints in `src/backend/aarch64/codegen/emit.cpp` directly to `c4c::backend::parse_backend_minimal_call_crossing_direct_call_module()`, and emitting the fast path from `c4c::backend::ParsedBackendMinimalCallCrossingDirectCallModuleView` itself. Shared regalloc synthesis on the backend-only path now keys from the parsed lowering-owned source SSA value instead of a synthetic adapter-only token, while the legacy-assisted regalloc path still binds the real LIR source value observed by regalloc. Existing structured call-crossing regressions, including helper-symbol and lowered source-value rename cases, continue to prove that the asm path stays on lowering-owned metadata without falling back to backend IR text.

Validation: Baseline `test_fail_before.log` remained the existing `2667` passed / `2` failed reference point for this plan. `cmake --build build -j8 --target backend_lir_adapter_aarch64_tests backend_lir_adapter_tests backend_ir_tests` passed. `ctest --test-dir build -R '^(backend_lir_adapter_aarch64_tests|backend_lir_adapter_tests|backend_ir_tests)$' --output-on-failure` passed for the touched AArch64 and nearby shared backend surfaces. Full-suite `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log` finished with the same `2667` passed / `2` failed summary, and `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed` passed with zero new failing tests. The only remaining failures are still the same two known AArch64 object-contract cases already recorded for this plan: `backend_contract_aarch64_return_add_object` and `backend_contract_aarch64_global_load_object`.

Next Slice Hint: Inspect `src/backend/ir.hpp`, `src/backend/lowering/call_decode.hpp`, and the remaining target emitters for the next parser-shaped backend surface that still blocks Step 3 backend-native structuring now that the direct-call asm fast paths, including AArch64 call-crossing, emit from shared backend views.

Iteration Target: Completed the AArch64 call-crossing direct-call cutover. Next iteration should identify the next backend-native structured surface needed for Step 3 or confirm whether Step 4 has any remaining emitter-local compatibility reshaping in the current migrated slice.
