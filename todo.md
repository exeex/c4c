Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Residual Failures

# Current Packet

## Just Finished

Step 2 - Classify Residual Failures: classified the fresh broad backend regex
evidence from `test_after.log` without changing implementation, tests,
expectations, runner behavior, timeout policy, or proof-log policy.

Current selected/pass/fail counts remain: 352 selected, 295 passed, 57 failed.
All 57 failures are external `c_testsuite_aarch64_backend_*` tests; local
backend/unit/CLI tests selected by `-R backend` passed.

Classified inventory:

- Local backend/unit/CLI failures: 0.
- AArch64 c-testsuite frontend/prepared-node machine-printer diagnostics: 2.
  Representatives:
  - `00164.c`: `FRONTEND_FAIL`; selected scalar `mul` cannot print because
    printable RHS facts are incomplete.
  - `00214.c`: `FRONTEND_FAIL`; selected scalar `add` cannot print because
    memory operands require prepared frame-slot sources.
- AArch64 c-testsuite semantic `lir_to_bir` admission diagnostics: 2.
  Representatives:
  - `00204.c`: no generated `.s`/`.bin`; `semantic lir_to_bir function
    'myprintf' failed in gep local-memory semantic family`.
  - `00216.c`: no generated `.s`/`.bin`; `semantic lir_to_bir function 'foo'
    failed in load local-memory semantic family`.
- AArch64 c-testsuite runtime nonzero or crash, quiet/return-code cases: 33.
  Tests: `00023.c`, `00026.c`, `00035.c`, `00051.c`, `00062.c`, `00066.c`,
  `00067.c`, `00068.c`, `00069.c`, `00070.c`, `00086.c`, `00089.c`,
  `00095.c`, `00096.c`, `00102.c`, `00110.c`, `00111.c`, `00112.c`,
  `00113.c`, `00119.c`, `00123.c`, `00137.c`, `00138.c`, `00142.c`,
  `00144.c`, `00151.c`, `00170.c`, `00173.c`, `00179.c`, `00207.c`,
  `00208.c`, `00215.c`, and `00200.c`.
  Evidence: runner reports `[RUNTIME_NONZERO]`; most have empty
  `stdout+stderr`, while `00200.c` prints many failed left-shift type rows
  before returning `exit=27`. Segfault representatives include `00089.c`,
  `00170.c`, `00173.c`, `00179.c`, `00207.c`, `00208.c`, and `00215.c`.
- AArch64 c-testsuite runtime mismatch/crash-output cases: 16.
  Tests: `00157.c`, `00159.c`, `00168.c`, `00169.c`, `00172.c`, `00174.c`,
  `00175.c`, `00176.c`, `00182.c`, `00185.c`, `00186.c`, `00193.c`,
  `00195.c`, `00196.c`, `00205.c`, and `00218.c`.
  Evidence: runner reports `[RUNTIME_MISMATCH]` with expected/actual output.
  Representative semantic shapes include local array stores/loads
  (`00157.c`, `00185.c`), direct-call/function result handling (`00159.c`),
  recursion/control-flow (`00168.c`, `00193.c`, `00196.c`), pointer/local
  address identity (`00172.c`), floating-point and conversion behavior
  (`00174.c`, `00175.c`, `00195.c`), global aggregate/array initialization or
  indexing (`00176.c`, `00205.c`), string/buffer formatting (`00182.c`,
  `00186.c`), and unsigned enum bit-field extension (`00218.c`).
- AArch64 c-testsuite output-storm plus runtime crash: 1.
  `00181.c` reports `[RUNTIME_NONZERO] exit=Segmentation fault` after a very
  large Tower-of-Hanoi output stream with corrupt tower values. Treat this as
  runtime/output-storm evidence, not a timeout-policy candidate.
- AArch64 c-testsuite timeouts: 3.
  `00143.c`, `00187.c`, and `00220.c` reached CTest's 5 second timeout. The
  current log contains no runner diagnostic body for these cases, so the
  missing fact is whether each timeout is a true generated-runtime hang, an
  output storm, or a slow external tool/runtime path.

Closed-owner non-reopen notes:

- Ideas 285 through 294: no local/backend unit regression or generated-code
  contradiction was observed in the current Step 2 evidence; do not reopen
  from the 57-count residual alone.
- Idea 296 fused compare-branch operand forms: no current residual carries
  the old fused compare-branch printer diagnostic; `00200.c` is now runtime
  nonzero with bad left-shift type rows.
- Idea 298 global/pointer/aggregate `lir_to_bir` projection: `00204.c` and
  `00216.c` still have `lir_to_bir` diagnostics, but the current missing facts
  are precise local-memory GEP/load admission failures, not evidence to reopen
  the closed projection owner by count alone.
- Idea 299 scalar immediate materialization: no old scalar immediate encoding
  diagnostic appears in the current log.
- Idea 300 scalar-cast printer forms: no old zero/sign-extend printer
  diagnostic appears; current `00035.c`, `00151.c`, and `00208.c` residuals
  are runtime failures.
- Idea 301 memory-store operand materialization: no old store-source/symbol
  store printer diagnostic appears; current `00176.c`, `00181.c`, and
  `00182.c` are runtime/output residuals and `00187.c` is a timeout.
- Idea 302 scalar machine-node operand forms: `00164.c` still has selected
  scalar `mul` printable-RHS evidence, so the missing fact is whether this is
  outside the closed scalar-node owner boundary or indicates a fresh
  prepared-fact regression; no implementation probe was performed in this
  inventory packet.
- Ideas 306, 307, and 308 address/materialization/PIC owners: no current
  assembler or linker legality diagnostic appears for symbol+offset width,
  large scalar immediate materialization, or external data-symbol PIC forms.
- Idea 309 indirect-call preservation: no current `00189.c` residual returns.
- Idea 311 call-boundary move preparation/printing: `00140.c` passes in the
  broad run and no old call-boundary selected-node diagnostic appears.

Ambiguous buckets and missing facts:

- Runtime nonzero and runtime mismatch buckets have generated `.s`/`.bin`
  artifacts, but the broad log alone does not identify the first bad backend
  fact. The missing fact is a narrow semantic probe per candidate bucket
  comparing prepared BIR/value homes/generated AArch64 for representative
  tests before splitting a focused owner.
- The timeout bucket lacks runner bodies. The missing fact is per-test timeout
  classification under stale-process hygiene: true hang, output storm, or slow
  runtime/toolchain path.
- `00164.c` and `00214.c` are the only current compile/printer diagnostics and
  are crisp enough for later owner selection, but this packet only records the
  diagnostics and does not decide the focused owner.

## Suggested Next

Select the next semantic owner from the classified inventory. The crispest
compile-stage candidates are the `00164.c` scalar `mul` printable-RHS fact gap,
the `00214.c` prepared frame-slot source gap, and the `00204.c`/`00216.c`
semantic `lir_to_bir` local-memory admission diagnostics; runtime buckets need
narrower generated-artifact probes before a focused owner is safe.

## Watchouts

- This was classification-only; no implementation files, tests, expectations,
  allowlists, unsupported classifications, runner behavior, timeout policy, or
  proof-log policy were changed.
- Do not reopen closed owners 285 through 311 from pass counts alone; require
  generated-code, diagnostic, or proof evidence that contradicts the closure
  boundary.
- `00181.c` produced a very large corrupt runtime output stream before
  segfaulting; keep it separate from the three CTest timeout cases.
- `00204.c` and `00216.c` have no generated `.s`/`.bin` artifacts in
  `build/c_testsuite_aarch64_backend/src/` because they stop at semantic
  `lir_to_bir` admission.
- Runtime buckets are not yet safe implementation owners without a narrow
  first-bad-fact probe.

## Proof

No new CTest run was required or performed for Step 2. Used the existing
fresh evidence log:

```bash
cd /workspaces/c4c/build && ctest -j10 -R backend --output-on-failure
```

Captured at repo root as `test_after.log` by Step 1.
Result in that log: CTest exited nonzero with 57 failures out of 352 selected
tests. The log is suitable classification evidence, not a green acceptance
proof.
