Status: Active
Source Idea Path: ideas/open/263_phase_f3_full_suite_timeout_baseline_regression_diagnosis.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Root-Cause Inspection

# Current Packet

## Just Finished

Completed Step 3 from `plan.md`, `Root-Cause Inspection`, as an evidence-only
history/log inspection for the non-reproduced
`c_testsuite_aarch64_backend_src_00040_c` timeout.

Candidate root causes inspected:

- Full-suite scheduling/runtime margin: strongest current classification.
  `test_baseline.new.log` was produced by the hook baseline command
  `ctest --test-dir build -j --output-on-failure` and records only a CTest
  timeout for `c_testsuite_aarch64_backend_src_00040_c` under a hard
  `TIMEOUT "5"`. The same case passed twice in narrow reruns at 2.84 and
  2.85 seconds, while the failed full-suite artifact shows higher aggregate
  load than the accepted baseline: total real time 38.97s vs 37.89s,
  `c_testsuite` 50.41s vs 47.61s, `aarch64_backend` 30.79s vs 28.86s, and
  `llvm_gcc_c_torture` 212.50s vs 206.14s.
- Recent implementation regression in the bounded window: not supported by the
  available evidence. The non-lifecycle implementation diff from
  `ab6efaf8a3af38595cd8a4bdb64932b49fcea680..b9d71a7a9e10554c0496c50e82209ac6a3db9d06`
  is limited to `src/backend/prealloc/publication_plans.cpp` and
  `src/backend/prealloc/select_chain_lookups.cpp`; no AArch64 emitter,
  c-testsuite registration, runner, timeout, target triple, or `00040.c`
  source files changed in the bounded window.
- Direct-global select-chain / recovered-store-source / byval pointer-source
  Phase F3 changes: plausible AArch64 backend surfaces in general, but no
  concrete link to this timeout is established from the artifacts. The failing
  source is the eight-queens `tests/c/external/c-testsuite/src/00040.c` case,
  dominated by recursion, array indexing, and runtime search; the archived
  baseline summary contains no compiler stderr, backend output, runtime stderr,
  or per-step timing showing one of those prepared lookup paths as the slow
  child process.

Step 3 conclusion: no concrete semantic/backend root cause is identified for a
code fix in this packet. The best-supported classification is environmental or
load-sensitive full-suite timeout margin, with missing raw full-suite evidence
preventing a final baseline decision.

## Suggested Next

Execute one supervisor-owned baseline-resolution packet: rerun the full-suite
baseline command that produced the candidate,
`ctest --test-dir build -j --output-on-failure`, and preserve raw output in a
supervisor-approved artifact before deciding whether to reject the stale
3427/3428 candidate or continue root-cause work.

## Watchouts

- Do not accept or delete `test_baseline.new.log` while it reports 3427/3428.
- Do not weaken timeout, expected output, supported status, labels, or test
  inclusion.
- The bounded regression window is still not exact: there are no full-suite
  baseline artifacts for intervening commits `728856001` through `b6353fe6d`.
- The archived candidate baseline is a formatted summary, not raw CTest output,
  so it cannot identify whether timeout time was spent in frontend codegen,
  assembly/link, runtime execution, or host scheduling delay.

## Proof

Evidence-only packet. No tests were run, and `test_after.log` was not created
or modified by this packet.

Inspected evidence paths:

- `test_baseline.log`
- `test_baseline.new.log`
- `log/baseline_ab6efaf8a3af38595cd8a4bdb64932b49fcea680.log`
- `log/baseline_b9d71a7a9e10554c0496c50e82209ac6a3db9d06.log`
- `test_before.log`
- `test_after.log`
- bounded git history and diffs for
  `ab6efaf8a3af38595cd8a4bdb64932b49fcea680..b9d71a7a9e10554c0496c50e82209ac6a3db9d06`
