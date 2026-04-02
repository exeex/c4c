Status: Active
Source Idea: ideas/open/02_backend_regression_repair_after_aarch64_refactor.md
Source Plan: plan.md

# Active Queue: Backend Regression Recovery

## Queue
- [x] Step 1: Baseline triage and ownership
  - Notes: Capture one representative failure per cluster and map root-cause ownership.
    - Status: Representative outputs captured and mapped to two clusters (aarch64 emit/IR seam, x86 PIE linkage).
  - Blockers: None.
- [x] Step 2: Repair aarch64 adapter seam
  - Notes: Implement and validate first targeted backend fix.
    - Active slice: Route `backend::emit_module(..., Target::Aarch64)` raw-LIR fallback through backend-IR seam so unsupported adapter cases stay in textual IR while supported bounded slices still emit asm.
    - Result: `backend_lir_adapter_aarch64_tests` now passes after restoring backend-IR fallback for unsupported raw-LIR cases, preserving bounded general-emitter asm slices, and updating the direct-emitter parameter-slot expectation to match the preserved-register lowering.
  - Blockers: None.
- [x] Step 3: Repair x86/runtime metadata-related regressions
  - Notes: Implement second slice fixes and targeted validation.
    - Result: `run_backend_positive_case.cmake` now routes `BACKEND_OUTPUT_KIND=asm` cases to a real assembler path, so backend runtime cases no longer hand `.ll` filenames to the asm flow.
    - Result: `src/apps/c4cll.cpp` now requests `-relocation-model=pic` when `llc` lowers x86_64 fallback IR to asm, eliminating the `.rodata.str1.1` absolute relocations that were breaking PIE links.
    - Targeted validation: `ctest --test-dir build -R 'backend_runtime_(nested_member_pointer_array|nested_param_member_array|param_member_array)|c_testsuite_x86_backend_src_(00025|00156)_c' --output-on-failure` passes.
  - Blockers: None.
- [x] Step 4: Cross-slice validation
  - Notes: Run full backend regression suite and classify remaining outcomes.
    - Current validation: `ctest --test-dir build -R backend --output-on-failure` now leaves only 2 failures:
      - `backend_contract_aarch64_return_add_object`
      - `backend_contract_aarch64_global_load_object`
    - Classification: the residual x86 backend runtime mismatch cluster (`00131`, `00165`, `00177`, `00188`, `00206`, `00211`) was fixed by stopping the x86 LIR adapter from collapsing multi-call direct vararg blocks to the final `printf`; the remaining two failures are AArch64 contract/object emission gaps that were already outside the repaired x86 cluster.
    - Verification:
      - `ctest --test-dir build -R backend_lir_adapter_x86_64_tests --output-on-failure` passes with the new multi-`printf` adapter regression.
      - `ctest --test-dir build -R 'c_testsuite_x86_backend_src_(00131|00165|00177|00188|00206|00211)_c' --output-on-failure` passes.
      - `build/c4cll --codegen asm --target x86_64-unknown-linux-gnu tests/c/external/c-testsuite/src/00188.c -o /tmp/00188.backend.s` now emits the full six-`printf` sequence via LLVM-IR fallback instead of a one-call truncated asm stub.
    - Note: archived `test_fail_before.log` / `test_fail_after.log` are truncated and cannot be parsed by `.codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py`, so this iteration used the live `ctest -R backend` sweep as the authoritative validation source.
  - Blockers: None.
- [ ] Step 5: Closeout and handoff
  - Notes: Record the repaired x86 multi-call vararg cluster in the source idea and decide whether the remaining two AArch64 contract failures should be closed as deferred baseline or taken as a follow-on backend slice.
  - Blockers: None.

Current Step: Step 5
Next Step: Classify/defer the two remaining AArch64 contract failures
  - Evidence captured:
    - `backend_runtime_nested_member_pointer_array` / `backend_runtime_nested_param_member_array` / `backend_runtime_param_member_array` (tests #168-#170): now pass after the positive backend harness writes asm cases to `.s` outputs.
    - `c_testsuite_x86_backend_src_00025_c` / `c_testsuite_x86_backend_src_00156_c` (tests #773/#1035): now pass after x86_64 fallback lowering switched to PIC-safe `llc` assembly.
    - `c_testsuite_x86_backend_src_00131_c` / `00165_c` / `00177_c` / `00188_c` / `00206_c` / `00211_c`: now pass after the x86 vararg-call adapter preserves multi-call blocks and lets unsupported multi-call surfaces fall back to LLVM IR lowering instead of emitting a truncated one-call asm stub.
    - `ctest --test-dir build -R backend --output-on-failure`: residual failures are now limited to `backend_contract_aarch64_return_add_object` and `backend_contract_aarch64_global_load_object`.
  - Slice mapping:
    - Step 3 slice fixed: x86 toolchain ABI/linkage cluster plus backend runtime asm-path emission.
    - Step 4 slice fixed: x86 multi-call direct-vararg lowering regression in the backend adapter path.
    - Remaining validation set: residual aarch64 object-contract drift.
