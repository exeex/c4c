Status: Active
Source Idea: ideas/open/03_lir_to_backend_ir_refactor.md
Source Plan: plan.md

# Active Queue: LIR To Backend IR Refactor

## Queue
- [x] Step 1: Re-establish the lowering boundary
  - Notes: Moved the production lowering declarations and implementation to `src/backend/lowering/lir_to_backend_ir.*`, kept `src/backend/lir_adapter.hpp` as the legacy shim, and validated the touched backend suites plus a monotonic full-suite regression check.
  - Blockers: None.
- [x] Step 2: Isolate LIR syntax decoding
  - Notes: Shared lowering helpers in `src/backend/lowering/call_decode.*` now own the staged LIR call and signature parsing that had been spread across x86 and AArch64 emitters. Targeted lowering and backend tests remained green, and full-suite regression checks stayed monotonic with the same two known AArch64 object-contract failures.
  - Blockers: None.
- [ ] Step 3: Make backend IR more backend-native
  - Notes: The latest slice tightens backend-owned structured `ptrdiff_eq` validation in `src/backend/ir_validate.cpp` so two `%local` addresses can no longer compare across different declared local-slot bases. Added backend-IR regression coverage in `tests/backend/backend_ir_tests.cpp` for a structured cross-local-slot ptrdiff module that declares `%lv.arr` and `%lv.other` and now rejects mixed-base ptrdiff with an explicit local-slot mismatch error. The previous slice extended structured local-slot ptrdiff validation to bounded same-base `%lv.arr` addresses, including out-of-bounds rejection once legacy ptrdiff type text is cleared. Earlier Step 3 slices moved more backend seams off raw signature, call, binary, and global-type text and onto structured metadata while keeping backend-scoped regressions stable.
  - Blockers: None.
- [ ] Step 4: Shift target codegen onto backend semantics
  - Notes: Defer until Step 3 lands enough structured operand and call surfaces.
  - Blockers: Depends on Step 3.
- [ ] Step 5: Validate positioning for later BIR work
  - Notes: Closeout only after the active structural slice lands and backend validation is stable.
  - Blockers: Depends on prior steps.

Current Step: Step 3
Next Step: Re-scan the remaining structured local-object seams for another mixed-provenance or unknown-local-base contract that still bypasses backend-owned metadata.
Blockers: None

Latest Iteration Note: `ptrdiff_eq` now rejects mixed-base structured local-slot comparisons by requiring two `%local` ptrdiff addresses to reference the same declared slot base before the structured same-base checks run.

Iteration Update: Structured local-slot addresses on the lowered local-array path now carry enough backend-owned provenance for both same-base and mixed-base ptrdiff checks, so validator can distinguish valid `%lv.arr + 0/4` arithmetic from both escaped `%lv.arr + 8` offsets and cross-slot `%lv.arr` vs `%lv.other` comparisons instead of relying only on local-looking symbols plus alignment.

Validation: `cmake -S . -B build`, `cmake --build build -j8`, `ctest --test-dir build -j --output-on-failure > test_before.log`, `cmake --build build -j8 --target backend_ir_tests`, `./build/backend_ir_tests`, `ctest --test-dir build -j --output-on-failure > test_after.log`, and `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed` passed at the monotonic full-suite guard level with `2667 passed / 2 failed` before and after, preserving the same existing failures: `backend_contract_aarch64_return_add_object` and `backend_contract_aarch64_global_load_object`.

Next Slice Hint: Re-scan the remaining structured pointer-shaped seams for any other mixed-provenance cases or local-object slices that still bypass backend-owned metadata, with special attention to unknown `%local` bases that may still be treated as structurally valid.
