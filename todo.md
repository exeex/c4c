Status: Active
Source Idea Path: ideas/open/279_aarch64_c_testsuite_llvm_stack_helper_symbols.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Broader AArch64 Backend Scan

# Current Packet

## Just Finished

Step 5 of `plan.md` ran the broader AArch64 backend scan and recorded
post-repair classification evidence for the LLVM stack-helper symbol family.

The broad `aarch64_backend` label selected 220 c-testsuite cases. Configure and
build succeeded; all 220 selected tests still failed, but none failed as
`[BACKEND_FAIL]` and no unresolved LLVM stack-helper symbol references remained
in `test_after.log`.

Post-repair stage counts:

- `[RUNTIME_UNAVAILABLE]`: 189
- `[FRONTEND_FAIL]`: 31
- `[BACKEND_FAIL]`: 0

Exact helper-symbol counts in `test_after.log`:

- `llvm.stacksave`: 0
- `llvm.dynamic_alloca.*`: 0
- `llvm.stackrestore`: 0

The representative `00207.c` case remains a truthful `[FRONTEND_FAIL]` with the
explicit dynamic-stack diagnostic:
`deferred_unsupported: AArch64 dynamic-stack helper lowering is not implemented; prepared dynamic-stack operations are rejected before unresolved helper calls reach machine output`.

Remaining follow-up failure families are outside this helper-symbol packet:
16 scalar machine-printer failures, 14 semantic `lir_to_bir` admission failures,
and the single truthful dynamic-stack unsupported diagnostic for `00207.c`.

## Suggested Next

Ask the supervisor or plan owner to run Step 6 completion review. The source
idea appears ready for closure review because the focused backend proof passed
earlier, the representative c-testsuite case no longer emits unresolved
`llvm.*` helper calls, and the broad AArch64 backend scan shows zero remaining
unresolved stack-helper symbol references. Do not count the 189
`[RUNTIME_UNAVAILABLE]` cases as pass evidence.

## Watchouts

- Do not count `[RUNTIME_UNAVAILABLE]` as pass evidence.
- Do not add filename-specific handling for `00207.c`.
- Do not weaken expectations, allowlists, unsupported classifications, or
  stage-specific diagnostics.
- This route chose truthful fail-closed rejection, not full AArch64 dynamic
  stack-pointer lowering; `00207.c` is therefore expected to remain
  `[FRONTEND_FAIL]` until a separate dynamic-stack lowering initiative exists.
- The broad scan still exits nonzero because all selected tests are either
  runtime-unavailable or blocked by separate frontend/backend capability gaps.
- Keep completed call-boundary move and `.LBB...` label fixes out of scope
  unless their exact old diagnostics return.

## Proof

Delegated proof command:

```sh
set -o pipefail; { cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -L aarch64_backend; } 2>&1 | tee test_after.log
```

Result:

- AArch64 scan configure passed
- `c4cll` build in `build-aarch64-scan` passed
- `ctest -L aarch64_backend` selected 220 tests
- 0 passed, 220 failed
- post-repair classifications: 189 `[RUNTIME_UNAVAILABLE]`, 31
  `[FRONTEND_FAIL]`, 0 `[BACKEND_FAIL]`
- exact unresolved helper-symbol references in `test_after.log`: 0
  `llvm.stacksave`, 0 `llvm.dynamic_alloca.*`, 0 `llvm.stackrestore`
- representative follow-up failures:
  - scalar machine-printer gaps: 16 cases, e.g. `00024.c`, `00027.c`,
    `00028.c`, `00029.c`, `00031.c`
  - semantic `lir_to_bir` admission gaps: 14 cases, e.g. `00046.c`,
    `00140.c`, `00143.c`, `00157.c`, `00176.c`
  - dynamic-stack unsupported diagnostic: `00207.c`

The delegated proof exits nonzero because the broader scan still has expected
non-owned failures. The targeted unresolved LLVM stack-helper symbol family is
absent from `test_after.log`, and `test_after.log` is the preserved proof log.
