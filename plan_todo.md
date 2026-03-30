# AArch64 Direct-Call Convergence Task List

Status: Active
Source Idea: ideas/open/16_aarch64_direct_call_convergence_plan.md
Source Plan: plan.md

## Todo

1. [ ] Verify direct-call parity matrix in `tests/backend/backend_lir_adapter_tests.cpp` (x86 vs aarch64)
2. [ ] Add AArch64 param-slot direct-call test (asm path, no fallback)
3. [ ] Add AArch64 counterpart rewrite/spacing variant if required
4. [ ] Register new test(s) in the test sequence
5. [ ] Run `ctest -j --output-on-failure -L aarch64_backend`
6. [ ] Record runbook outcome and remaining gaps

