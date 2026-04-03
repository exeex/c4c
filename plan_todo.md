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
  - Notes: First Step 4 slice is in flight on x86: the zero-argument direct-call asm path now consumes `ParsedBackendMinimalDirectCallModuleView` directly instead of rebuilding a target-local compatibility slice, and the tightened x86 regression also renames the lowered call result plus return wiring to prove the fast path follows structured SSA relationships rather than fixed text names.
  - Blockers: Keep additional emitter cutovers behind the remaining Step 3 structured-surface work.
- [ ] Step 5: Validate positioning for later BIR work
  - Notes: Closeout only after the active structural slice lands and backend validation is stable.
  - Blockers: Depends on prior steps.

Current Step: Step 4
Next Step: If regression coverage stays clean, either mirror the same zero-argument direct-call emitter cutover on AArch64 or move the next x86 direct-call family member (`direct_call_add_imm` or `two_arg_direct_call`) onto direct structured parsed-module views.
Blockers: None

Latest Iteration Note: The first Step 4 emitter cutover is now on x86: the zero-argument direct-call asm path consumes lowering-owned structured module metadata directly, and the tightened regression proves the path survives renamed callee symbols together with renamed call-result and return-value wiring.

Iteration Update: Tightened `test_x86_backend_scaffold_accepts_renamed_structured_zero_arg_direct_call_ir_without_signature_shims()` so the lowered backend call result and matching return SSA edge are renamed along with the callee symbol, then removed the x86-local zero-argument direct-call reshaping layer by emitting from `c4c::backend::ParsedBackendMinimalDirectCallModuleView` directly in `src/backend/x86/codegen/emit.cpp`.

Validation: Baseline `test_fail_before.log` remained the existing `2667` passed / `2` failed reference point for this plan. `cmake --build build -j8 --target backend_lir_adapter_tests backend_lir_adapter_aarch64_tests backend_lir_adapter_x86_64_tests` passed. `ctest --test-dir build -R '^backend_lir_adapter_tests$' --output-on-failure`, `ctest --test-dir build -R '^backend_lir_adapter_aarch64_tests$' --output-on-failure`, and `ctest --test-dir build -R '^backend_lir_adapter_x86_64_tests$' --output-on-failure` all passed for this slice. Full-suite `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log` finished with the same `2667` passed / `2` failed summary, and `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed` passed with zero new failing tests. The only remaining failures are still the same two known AArch64 object-contract cases already recorded for this plan: `backend_contract_aarch64_return_add_object` and `backend_contract_aarch64_global_load_object`.

Next Slice Hint: Verify whether any direct-call-family matcher or legacy-fallback seam still duplicates lowering-owned shape checks; if not, start the first Step 4 emitter cutover by replacing one compatibility parse path with direct consumption of structured backend semantics.
