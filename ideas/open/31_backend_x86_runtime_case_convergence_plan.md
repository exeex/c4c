# x86 Runtime Case Convergence Plan

## Relationship To Roadmap

Umbrella source: `ideas/open/__backend_port_plan.md`

Depends on:

- `ideas/closed/15_backend_x86_port_plan.md`
- `ideas/closed/03_backend_regalloc_peephole_port_plan.md`

Should precede:

- broader x86 built-in assembler work
- broader x86 built-in linker work
- any x86 plan that assumes helper calls, parameter passing, and bounded branch/local/global seams already stay on the backend-owned asm path

## Goal

Drive more real x86 backend runtime cases from partial asm coverage to stable backend-owned asm execution, one bounded testcase family at a time, without widening into assembler/linker parity work.

## Current Verified State

As of 2026-03-29, the x86 backend already passes a bounded set of real `.c` cases through the backend-owned asm path:

- `tests/c/internal/backend_case/return_zero.c`
- `tests/c/internal/backend_case/return_add.c`
- `tests/c/internal/backend_case/local_array.c`

Focused validation also shows that some adjacent x86 seams are still incomplete:

- `tests/c/internal/backend_case/call_helper.c` still falls back to LLVM IR instead of staying on the backend-owned asm path
- the failure mode is structural and easy to detect: the runtime harness requests `BACKEND_OUTPUT_KIND=asm`, but the compiler still emits LLVM text for that case

## Why This Is Separate

- the initial x86 port plan is already closed and should not silently absorb a new round of testcase-driven convergence work
- this next round is not about file translation anymore; it is about turning more real testcase families into backend-owned asm slices
- helper calls, parameter lowering, bounded branches, local-slot materialization, and narrow global-addressing cases are adjacent but still distinct enough that they should be advanced in a controlled order

## Scope

### Primary convergence targets

- direct helper-call runtime cases
- single- and two-argument parameter passing cases
- bounded compare-and-branch runtime cases
- local-slot and local-address runtime cases that are already close to the explicit x86 backend seam
- the smallest backend-owned global-addressing cases only after helper-call and parameter seams are stable

### Required work shape

- keep using real runtime `.c` cases as the primary proof target
- pair each promoted runtime case with the narrowest backend adapter / emitter assertion that proves the exact seam
- treat LLVM-text fallback as a bug for any testcase explicitly in scope for this plan

## Explicit Non-Goals

- full x86 assembler completeness
- full x86 linker completeness
- broad x86 optimization work beyond what the promoted testcase slices require
- silently widening into general pointer arithmetic or ABI-completeness work without splitting follow-on ideas

## Suggested Execution Order

1. promote `call_helper.c` onto the x86 backend-owned asm path and stop accepting LLVM-text fallback for that slice
2. promote the narrowest parameter-passing runtime cases next:
   `param_slot`,
   `two_arg_helper`,
   `two_arg_local_arg`,
   `two_arg_second_local_arg`
3. tighten bounded compare-and-branch runtime coverage only after the helper-call and parameter seams are explicit
4. promote the smallest adjacent local/global address-formation runtime cases only when they no longer need broad new lowering machinery
5. split again if one testcase family starts requiring a separate mechanism family such as generic global-address arithmetic, wide atomics, or built-in assembler/linker expansion

## Validation

- `backend_lir_adapter_tests` should stop accepting LLVM fallback for each newly promoted x86 slice
- targeted runtime tests under `tests/c/internal/backend_case/` should pass with `BACKEND_OUTPUT_KIND=asm`
- the compiler output for promoted x86 cases should be assembly text, not LLVM IR
- compile/run verification should always include at least one real emitted `.s -> binary -> exit code` path for the active testcase family

## Good First Patch

Make `tests/c/internal/backend_case/call_helper.c` stay on the x86 backend-owned asm path end to end, then prove it with both backend adapter assertions and the runtime harness.
