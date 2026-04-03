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
  - Notes: The latest slice extended `ParsedBackendMinimalCallCrossingDirectCallModuleView` with lowering-owned `regalloc_source_value` metadata sourced from the structured backend `source_add` SSA name, updated shared parser regression coverage in `tests/backend/backend_lir_adapter_tests.cpp` to assert that preserved metadata explicitly, and rewired the x86 call-crossing asm path to consume the structured source name instead of reparsing legacy LIR just to recover regalloc ownership. The x86 synthesized backend-only regalloc path now keys the preserved call-crossing source register from that structured metadata too, so renamed lowered call-crossing seams continue to stay on the asm path without fallback. The prior slice removed the duplicate AArch64-local legacy call-crossing regalloc-source parser from `src/backend/aarch64/codegen/emit.cpp` and rewired that fallback to reuse the shared lowering-owned `parse_backend_single_helper_call_crossing_source_value()` helper that x86 already consumed. Shared regression coverage in `tests/backend/backend_lir_adapter_tests.cpp` had already proved the helper preserves renamed helper parameter names plus renamed source, call, and return SSA values for the legacy call-crossing seam. Earlier Step 3 slices introduced `parse_backend_minimal_declared_direct_call_module()` in `src/backend/lowering/call_decode.hpp`, rewired the x86 declared-direct-call seam in `src/backend/x86/codegen/emit.cpp` to consume that lowering-owned backend-module matcher, and centralized the structured direct-call matcher family behind lowering-owned decode.
  - Blockers: None.
- [ ] Step 4: Shift target codegen onto backend semantics
  - Notes: Step 4 x86 cutovers continue to replace emitter-local compatibility reshaping with direct lowering-owned parsed-module views. The zero-argument direct-call asm path consumes `ParsedBackendMinimalDirectCallModuleView` directly, `direct_call_add_imm` now emits from `ParsedBackendMinimalDirectCallAddImmModuleView`, and the earlier x86 slice removed the backend-IR `two_arg_direct_call` reshaping layer so the x86 asm fast path emits directly from `ParsedBackendMinimalTwoArgDirectCallModuleView`. This iteration removes the x86 backend-only declared-direct-call reshaping shim too, so `src/backend/x86/codegen/emit.cpp` now emits that asm path directly from `ParsedBackendMinimalDeclaredDirectCallModuleView` instead of copying lowering-owned metadata into a target-local slice. The remaining x86-only LIR `extern_decl` fast path still exists as a legacy compatibility entrypoint, but the backend-IR path now stays entirely on lowering-owned declared-call metadata. The newer AArch64 slices now mirror that pattern for the backend-owned structured direct-call family: `src/backend/aarch64/codegen/emit.cpp` first dropped the target-local `direct_call_add_imm` wrapper, then removed the backend-IR `two_arg_direct_call` reshaping layer, then removed the folded two-argument compatibility wrapper, then removed the zero-argument direct-call compatibility wrapper, and the latest slice removes the remaining AArch64-local `MinimalCallCrossingDirectCallSlice` reshaping layer so the call-crossing asm fast path now emits directly from `ParsedBackendMinimalCallCrossingDirectCallModuleView` while keying shared regalloc from lowering-owned source metadata.
  - Blockers: Keep additional emitter cutovers behind the remaining Step 3 structured-surface work.
- [ ] Step 5: Validate positioning for later BIR work
  - Notes: Closeout only after the active structural slice lands and backend validation is stable.
  - Blockers: Depends on prior steps.

Current Step: Step 4
Next Step: Re-scan the remaining target emitter compatibility seams and backend-native parser-shaped surfaces to find the next slice that can either remove a target-local reshaping layer or extend shared lowering-owned metadata without broadening the active plan.
Blockers: None

Latest Iteration Note: The current Step 4 slice is now landed: the x86 backend-only declared-direct-call asm path no longer copies lowering-owned parsed metadata into a target-local compatibility struct. `src/backend/x86/codegen/emit.cpp` now emits that backend path directly from `ParsedBackendMinimalDeclaredDirectCallModuleView`, and regression coverage proves both the explicit x86 emit surface and the target-selected backend entrypoint keep structured declared direct calls on assembly output.

Iteration Update: Removed the x86 backend-only declared-direct-call reshaping helper from `src/backend/x86/codegen/emit.cpp`, added an emitter overload that consumes `ParsedBackendMinimalDeclaredDirectCallModuleView` directly, switched both backend-module x86 declared-call entrypoints to the shared lowering-owned parser, and extended `tests/backend/backend_lir_adapter_x86_64_tests.cpp` so the explicit x86 emit surface regression locks that backend-only path onto assembly output.

Validation: Baseline `test_fail_before.log` remained the existing `2667` passed / `2` failed reference point for this plan. `cmake --build build -j8 --target backend_lir_adapter_tests backend_lir_adapter_x86_64_tests` passed. `ctest --test-dir build -R '^(backend_lir_adapter_tests|backend_lir_adapter_x86_64_tests)$' --output-on-failure` passed for the touched shared backend and x86 surfaces. Full-suite `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log` finished with the same `2667` passed / `2` failed summary, and `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed` passed with zero new failing tests. The only remaining failures are still the same two known AArch64 object-contract cases already recorded for this plan: `backend_contract_aarch64_return_add_object` and `backend_contract_aarch64_global_load_object`.

Next Slice Hint: Inspect `src/backend/x86/codegen/emit.cpp`, `src/backend/aarch64/codegen/emit.cpp`, `src/backend/ir.hpp`, and `src/backend/lowering/call_decode.hpp` for the next emitter-local compatibility wrapper or parser-shaped backend surface that can move behind a shared lowering-owned parsed module view.

Iteration Target: Completed the x86 backend-only structured declared-direct-call cutover. Next iteration should identify the next backend-native structured surface or remaining emitter-local compatibility seam worth moving behind shared lowering-owned metadata.
