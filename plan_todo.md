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
Next Step: Audit the remaining handwritten-asm matcher seams in `src/backend/x86/codegen/emit.cpp` that still keep helper-shape recognition local, especially the add-immediate helper matcher used by the structured direct-call path, then lift the next narrow helper contract into `src/backend/lowering/call_decode.hpp`.
Blockers: None

Latest Iteration Note: Shared lowering-owned parsing now also covers the declared direct-call backend-module contract itself. The x86 emitter no longer re-derives the backend-module shape for bounded extern/declared direct calls and instead consumes `parse_backend_minimal_declared_direct_call_module()` directly.

Iteration Update: Added `parse_backend_minimal_declared_direct_call_module()` to `src/backend/lowering/call_decode.hpp`, rewired `src/backend/x86/codegen/emit.cpp` to consume the shared lowering-owned declared direct-call parser instead of re-checking the backend-module shape locally, and added shared regression coverage in `tests/backend/backend_lir_adapter_tests.cpp` proving the declared-direct-call seam preserves renamed declaration symbols, renamed parameter names, direct-call operands, vararg signatures, and fixed return immediates without target-local backend-module scans.

Validation: Baseline `test_fail_before.log` remained the existing `2667` passed / `2` failed reference point for this plan. `cmake --build build -j8 --target backend_lir_adapter_tests backend_lir_adapter_x86_64_tests` passed. `ctest --test-dir build -R '^backend_lir_adapter_tests$' --output-on-failure` and `ctest --test-dir build -R '^backend_lir_adapter_x86_64_tests$' --output-on-failure` passed for this slice. Full-suite `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log` finished with the same `2667` passed / `2` failed summary, and `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed` passed with zero new failing tests. The only remaining failures are still the same two known AArch64 object-contract cases already recorded for this plan: `backend_contract_aarch64_return_add_object` and `backend_contract_aarch64_global_load_object`.

Next Slice Hint: Audit the remaining x86 handwritten-asm matcher helpers for places that still re-check structured backend helper shapes locally even though lowering already owns comparable contracts, then centralize the narrowest remaining seam with shared regression coverage.
