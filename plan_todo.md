Status: Active
Source Idea: ideas/open/02_backend_regression_repair_after_aarch64_refactor.md
Source Plan: plan.md

# Active Queue: Backend Regression Recovery

## Queue
- [x] Step 1: Baseline triage and ownership
  - Notes: Capture one representative failure per cluster and map root-cause ownership.
    - Status: Representative outputs captured and mapped to two clusters (aarch64 emit/IR seam, x86 PIE linkage).
  - Blockers: None.
- [ ] Step 2: Repair aarch64 adapter seam
  - Notes: Implement and validate first targeted backend fix.
  - Blockers: None.
- [ ] Step 3: Repair x86/runtime metadata-related regressions
  - Notes: Implement second slice fixes and targeted validation.
  - Blockers: None.
- [ ] Step 4: Cross-slice validation
  - Notes: Run full backend regression suite and classify remaining outcomes.
  - Blockers: None.
- [ ] Step 5: Closeout and handoff
  - Notes: Record outcomes in source idea and confirm scope exit criteria.
  - Blockers: None.

Current Step: Step 2
Next Step: Step 1
  - Evidence captured:
    - `backend_lir_adapter_aarch64_tests` (test #125): all sub-checks fail in emitted aarch64 output; runtime/member test IR is present, and casts/stack-prologue behavior diverges from expected, indicating backend IR seam and target-local lowering regressions.
    - `backend_runtime_nested_member_pointer_array` / `backend_runtime_nested_param_member_array` / `backend_runtime_param_member_array` (tests #168-#170): toolchain rejects generated `.ll` due to non-LLVM tokens and register/operand rendering, consistent with emitter/type-layout corruption from the same backend seam.
    - `c_testsuite_x86_backend_src_00025_c` / `c_testsuite_x86_backend_src_00156_c` (tests #773/#1035): backend emits non-PIE-compatible relocations (`R_X86_64_32(S)`), linking fails in x86 test toolchain.
  - Slice mapping:
    - AArch64 adapter/IR seam cluster: test #125 + related backend_runtime member-IR failures where IR text is malformed for Clang.
    - x86 toolchain ABI/linkage cluster: `c_testsuite_x86_backend_src_*` failures (PIE relocation class).
