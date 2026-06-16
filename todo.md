Status: Active
Source Idea Path: ideas/open/285_aarch64_llvm_path_fp128_vararg_codegen_crash_triage.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Reproduce And Reduce The Three Crash Families

# Current Packet

## Just Finished

Lifecycle activation created the active runbook from
`ideas/open/285_aarch64_llvm_path_fp128_vararg_codegen_crash_triage.md`.

## Suggested Next

Execute Step 1: reproduce the three AArch64 LLVM-path clang backend crashes,
capture the relevant IR and stderr evidence, reduce the triggering constructs,
and classify whether the failures share one family or split into separate
repair/boundary paths.

## Watchouts

- Do not downgrade the tests to unsupported without reduced evidence and
  supervisor approval.
- Do not suppress stderr or hide clang backend crashes behind missing-binary
  runtime failures.
- Do not special-case the three filenames, symbols, or known crash functions.
- Keep C++ dependent casts and C aggregate function-pointer ABI work out of
  this plan.

## Proof

Lifecycle-only activation; no build or CTest proof required.
