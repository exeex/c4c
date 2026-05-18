Status: Active
Source Idea Path: ideas/open/280_aarch64_dynamic_stack_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce And Trace Dynamic-Stack Records

# Current Packet

## Just Finished

Lifecycle activation created this execution state for Step 1 of `plan.md`.

## Suggested Next

Run Step 1: reproduce `c_testsuite_aarch64_backend_src_00207_c`, confirm the
current dynamic-stack unsupported diagnostic, and trace the prepared
dynamic-stack records consumed by AArch64 lowering.

## Watchouts

- Do not count `[RUNTIME_UNAVAILABLE]` as pass evidence.
- Do not restore unresolved `llvm.stacksave`, `llvm.dynamic_alloca.*`, or
  `llvm.stackrestore` references in generated AArch64 output.
- Do not weaken expectations, allowlists, unsupported classifications, or stage
  accounting to claim progress.

## Proof

Lifecycle-only activation; no build or test proof required.
