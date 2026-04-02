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
  - Notes: The latest slice teaches `src/backend/ir_validate.cpp` to carry explicit referenced-object kinds for globals, string constants, and `local_slots` during validation instead of partially classifying provenance from raw `%`-prefixed symbol spelling. Local-slot load/store mismatches now diagnose against local-slot element metadata directly, and `tests/backend/backend_ir_tests.cpp` covers those structured local-slot type mismatches. The previous slice tightened backend-owned local-object validation so declared `local_slots` must carry an explicit `element_type` instead of leaving typed provenance implicit in byte counts or `%local` symbol spelling, and earlier Step 3 slices moved more backend seams off raw signature, call, binary, and global-type text and onto structured metadata while keeping backend-scoped regressions stable.
  - Blockers: None.
- [ ] Step 4: Shift target codegen onto backend semantics
  - Notes: Defer until Step 3 lands enough structured operand and call surfaces.
  - Blockers: Depends on Step 3.
- [ ] Step 5: Validate positioning for later BIR work
  - Notes: Closeout only after the active structural slice lands and backend validation is stable.
  - Blockers: Depends on prior steps.

Current Step: Step 3
Next Step: Re-scan the remaining structured pointer and local-object seams for any other places where backend validation or codegen still infer provenance or object class from raw symbol spelling instead of structured local-slot, string-constant, or global records.
Blockers: None

Latest Iteration Note: Backend IR validation now threads explicit referenced-object kind metadata through globals, string constants, and local slots, so local-slot access diagnostics and ptrdiff object-class checks no longer depend on raw symbol-prefix heuristics.

Iteration Update: Structured local-slot addresses already carried explicit slot bounds and element metadata. This slice makes the validator carry explicit object kind metadata alongside those bounds, so load/store scalar-type mismatches are diagnosed against local-slot records directly and ptrdiff object-class checks stop consulting `%`-prefix shape once symbols are resolved. Added backend IR regression coverage for mismatched local-slot load/store types to pin that contract down.

Validation: `cmake -S . -B build`, `cmake --build build -j8 --target backend_ir_tests`, `./build/backend_ir_tests`, `cmake --build build -j8`, `ctest --test-dir build -j --output-on-failure > test_before.log`, and `ctest --test-dir build -j --output-on-failure > test_after.log` completed for the touched slice with the same full-suite steady state before and after of `2667 passed / 2 failed`, preserving the known failures `backend_contract_aarch64_return_add_object` and `backend_contract_aarch64_global_load_object`.

Next Slice Hint: Re-scan the remaining structured pointer-shaped seams for any other provenance gaps that still bypass backend-owned metadata, especially codegen or validator paths that may still special-case raw symbol spelling instead of structured local-slot, string-constant, or global records.
