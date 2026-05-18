Status: Active
Source Idea Path: ideas/open/276_aarch64_c_testsuite_backend_runtime_execution.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Current Runner And Registration

# Current Packet

## Just Finished

Activated the AArch64 c-testsuite backend runtime execution plan.

## Suggested Next

Execute Step 1: audit the existing frontend c-testsuite LLVM IR flow, the
backend branches in `tests/c/external/c-testsuite/RunCase.cmake`, the
AArch64-specific runner in
`tests/c/external/c-testsuite-aarch64-backend-runner.cmake`, and registration
in `tests/c/external/CMakeLists.txt`.

The audit should decide whether the first implementation packet edits CMake
registration, runner diagnostics/contract, or proof targets.

## Watchouts

- A passing test must use AArch64 backend `.s`, not LLVM IR fallback.
- Runtime-unavailable is not a pass.
- Do not weaken expected outputs or shrink allowlists to get green.
- Do not add named-case AArch64 lowering shortcuts while working on the runner.

## Proof

Lifecycle-only activation. No build or test proof has been run yet.
