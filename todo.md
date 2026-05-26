Status: Active
Source Idea Path: ideas/open/18_aarch64_cts_00181_runtime_regression_reopen.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce and classify the current failure

# Current Packet

## Just Finished

Lifecycle activation created `plan.md` and this executor-compatible `todo.md`
from `ideas/open/18_aarch64_cts_00181_runtime_regression_reopen.md`.

## Suggested Next

Execute Step 1 from `plan.md`: reproduce
`c_testsuite_aarch64_backend_src_00181_c`, capture the current
`[RUNTIME_NONZERO]` segmentation fault details, and classify the likely failing
AArch64 capability surface before editing implementation code.

## Watchouts

- Do not special-case `00181`, Tower of Hanoi symbols, or one exact global
  array shape.
- Do not weaken or disable c_testsuite expectations.
- Do not restore broad target-local fallback selection as a substitute for
  complete prepared facts.
- Keep unrelated AArch64 calls, dispatch, and c_testsuite cleanup outside this
  route.

## Proof

Lifecycle-only activation; no build or test proof required.
