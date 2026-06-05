# 112 AArch64 00216/00204 Post-Closure Regression

## Goal

Repair the current AArch64 full-suite regression where
`c_testsuite_aarch64_backend_src_00216_c` and
`c_testsuite_aarch64_backend_src_00204_c` both segfault after idea 110 was
closed as baseline-clean.

## Why This Exists

Idea 110 closure states that commit `23213dbe4` had a full-suite
`test_baseline.new.log` result of 3427/3427 passing, with `00204` confirmed
stale-only in the older preserved baseline.  The latest full-suite baseline
candidate at commit `0be172fb0` reports only two failures:

- `1909 - c_testsuite_aarch64_backend_src_00216_c`
- `1919 - c_testsuite_aarch64_backend_src_00204_c`

A current focused rerun confirms both tests fail with runtime segmentation
faults and no stdout:

```sh
ctest --test-dir build -j1 --output-on-failure -R '^(c_testsuite_aarch64_backend_src_00(216|204)_c)$'
```

This makes the issue a post-closure regression to investigate from the
`23213dbe4..HEAD` change range, especially the call-planning frame-address
materialization and store-source publication visibility work that landed after
the baseline-clean closure.

## In Scope

- Reproduce the two current segfaults at `HEAD`.
- Compare behavior against the last baseline-clean closure point recorded by
  idea 110.
- Inspect the `23213dbe4..HEAD` change range for shared causes in:
  - `src/backend/prealloc/call_plans.cpp`
  - `src/backend/prealloc/publication_plans.cpp`
  - stack-layout prepared frame-slot materialization changes
  - prepared-printer/store-source visibility changes only if they influenced
    prepared-module construction rather than printing
- Determine whether `00216` and `00204` share a call aggregate address,
  variadic aggregate, frame-address materialization, or publication-plan
  authority defect.
- Fix the semantic regression without weakening test expectations.

## Out Of Scope

- Reopening all of idea 110's recovered failures.
- Broad BIR/prealloc residue cleanup unrelated to the two segfaults.
- x86 or RISC-V work.
- Printer-only formatting changes unless they reveal a real construction-time
  side effect.
- Testcase-shaped shortcuts that special-case `00216.c` or `00204.c`.

## Acceptance And Completion Criteria

- The focused two-test command passes.
- The previous idea-110 recovered target set remains clean, at minimum:

```sh
ctest --test-dir build -j1 --output-on-failure -R '^(backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00(172|180|216|220|204)_c)$'
```

- Broader AArch64 or full-suite baseline validation has no unexplained new
  failures before closure.
- Closure notes identify the exact regression source and whether `00216` and
  `00204` shared a root cause.

## Reviewer Reject Signals

- The fix disables, skips, or weakens either C testsuite case.
- The implementation matches the filenames or expected-output shape instead of
  repairing a lowering/data-flow rule.
- The route claims success from the two tests while reintroducing any of the
  idea-110 recovered failures.
- The change hides a missing prepared materialization or publication fact behind
  a legacy name fallback without documenting and bounding the compatibility
  path.
- The change treats prepared-printer output as the root cause without proving a
  construction-time side effect.

## Closure Notes

Closed after the focused regressions and the prior idea-110 recovered target
set were repaired without weakening test expectations. The final close guard
used matching canonical AArch64-labelled CTest logs:

```sh
ctest --test-dir build -j --output-on-failure -L aarch64
```

`test_before.log` recorded 270/272 passing with the two byval stack-overflow
route probes failing under the old outgoing-stack contract. `test_after.log`
recorded 272/272 passing after the contract repair, and
`c4c-regression-guard` reported no new failures.

The repaired failure family was call aggregate and call-boundary related, but
`00216` and `00204` did not share one identical root cause. `00216` was fixed
by restoring direct aggregate address materialization for frame-slot-backed
local aggregate pointer operands. `00204` additionally required outgoing
AArch64 stack argument reservation before `x16`-relative stores and stable
callee-side `va_list` home preservation for aggregate `va_arg`.
