# AArch64 Direct-Call Convergence — Execution State

Source Idea: ideas/open/16_aarch64_direct_call_convergence_plan.md

## Completed
- [x] Step 1: Confirm baseline and scope lock
  - Baseline: 2534/2671 (137 failed)
  - x86/aarch64 direct-call parity is near-complete
  - Gaps identified:
    1. `test_aarch64_backend_renders_param_slot_direct_call_slice` — missing helper body asm verification
    2. `test_aarch64_backend_renders_typed_direct_call_local_arg_slice` — missing `target triple =` non-fallback assertion
    3. Non-direct-call gaps (goto_only, countdown_while, countdown_do_while) out of scope
- [x] Step 2: Enrich param-slot and local-arg aarch64 tests for full x86 parity
  - Added helper body assertion `add_one:\n  add w0, w0, #1\n  ret\n` to param_slot_direct_call_slice
  - Added `expect_not_contains(rendered, "target triple =", ...)` to typed_direct_call_local_arg_slice
  - All four completion criteria pass: helper symbol, bl, ABI register arg, no LLVM fallback
- [x] Step 3: Call-rewrite symmetry — no-op, all x86 rewrite variants already have aarch64 counterparts
- [x] Step 4: Validate with ctest — 2534/2671 (no regression from baseline)
- [x] Step 5: Regression report
  - Direct-call symmetry: fully converged
  - Intentionally deferred: goto_only_constant_return, countdown_while_return, countdown_do_while_return (non-direct-call, out of scope)
  - No accidental x86-only or aarch64-only behavior claims in the direct-call set
