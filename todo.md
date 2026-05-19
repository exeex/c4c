Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Residual Buckets

# Current Packet

## Just Finished

Step 2 from `plan.md` classified the supervisor-created fresh post-307
backend-regex capture in `test_before.log`. No tests were rerun and no
implementation, test, expectation, allowlist, CTest, runner, timeout, or
proof-log policy files were changed.

Fresh capture counts: `ctest -R backend` selected 352 tests, passed 306, and
reported 46 failing tests. Of the 46 residuals, 43 are ordinary failed tests
and 3 are timeouts. All residuals are `c_testsuite_aarch64_backend_*`; local
backend/unit/CLI tests selected by `-R backend` passed.

Failing tests from the fresh capture:

- `c_testsuite_aarch64_backend_src_00035_c`
- `c_testsuite_aarch64_backend_src_00066_c`
- `c_testsuite_aarch64_backend_src_00086_c`
- `c_testsuite_aarch64_backend_src_00089_c`
- `c_testsuite_aarch64_backend_src_00102_c`
- `c_testsuite_aarch64_backend_src_00111_c`
- `c_testsuite_aarch64_backend_src_00112_c`
- `c_testsuite_aarch64_backend_src_00113_c`
- `c_testsuite_aarch64_backend_src_00119_c`
- `c_testsuite_aarch64_backend_src_00123_c`
- `c_testsuite_aarch64_backend_src_00137_c`
- `c_testsuite_aarch64_backend_src_00138_c`
- `c_testsuite_aarch64_backend_src_00140_c`
- `c_testsuite_aarch64_backend_src_00143_c`
- `c_testsuite_aarch64_backend_src_00144_c`
- `c_testsuite_aarch64_backend_src_00151_c`
- `c_testsuite_aarch64_backend_src_00157_c`
- `c_testsuite_aarch64_backend_src_00159_c`
- `c_testsuite_aarch64_backend_src_00164_c`
- `c_testsuite_aarch64_backend_src_00168_c`
- `c_testsuite_aarch64_backend_src_00169_c`
- `c_testsuite_aarch64_backend_src_00170_c`
- `c_testsuite_aarch64_backend_src_00172_c`
- `c_testsuite_aarch64_backend_src_00173_c`
- `c_testsuite_aarch64_backend_src_00174_c`
- `c_testsuite_aarch64_backend_src_00175_c`
- `c_testsuite_aarch64_backend_src_00176_c`
- `c_testsuite_aarch64_backend_src_00179_c`
- `c_testsuite_aarch64_backend_src_00181_c`
- `c_testsuite_aarch64_backend_src_00182_c`
- `c_testsuite_aarch64_backend_src_00185_c`
- `c_testsuite_aarch64_backend_src_00186_c`
- `c_testsuite_aarch64_backend_src_00187_c`
- `c_testsuite_aarch64_backend_src_00189_c`
- `c_testsuite_aarch64_backend_src_00193_c`
- `c_testsuite_aarch64_backend_src_00195_c`
- `c_testsuite_aarch64_backend_src_00196_c`
- `c_testsuite_aarch64_backend_src_00200_c`
- `c_testsuite_aarch64_backend_src_00204_c`
- `c_testsuite_aarch64_backend_src_00207_c`
- `c_testsuite_aarch64_backend_src_00208_c`
- `c_testsuite_aarch64_backend_src_00214_c`
- `c_testsuite_aarch64_backend_src_00215_c`
- `c_testsuite_aarch64_backend_src_00216_c`
- `c_testsuite_aarch64_backend_src_00218_c`
- `c_testsuite_aarch64_backend_src_00220_c`

Bucketed residual evidence:

- `FRONTEND_FAIL` machine-printer/prepared-node residuals, 3 tests:
  `00140`, `00164`, `00214`.
  `00140` reaches machine printing with a deferred unsupported call-boundary
  move outside the selected register call-boundary move subset. `00164`
  reaches scalar `mul` printing with incomplete printable RHS facts. `00214`
  reaches scalar `add` printing with memory operands that require prepared
  frame-slot sources.
- `FRONTEND_FAIL` semantic `lir_to_bir` admission residuals, 2 tests:
  `00204`, `00216`.
  `00204` fails in `myprintf` in the `gep local-memory` semantic family.
  `00216` fails in `foo` in the `load local-memory` semantic family. These
  share the admission gate but not yet a proven single lowering owner.
- `BACKEND_FAIL` externally binding symbol/PIC relocation residual, 1 test:
  `00189`.
  Linker diagnostics report non-PIC `R_AARCH64_ADR_PREL_PG_HI21` relocation
  against externally binding `stdout@@GLIBC_2.17`; generated assembly forms
  `adrp/add` directly against `stdout`.
- `RUNTIME_NONZERO`, 22 tests:
  `00035`, `00066`, `00086`, `00089`, `00102`, `00111`, `00112`, `00113`,
  `00119`, `00123`, `00137`, `00138`, `00144`, `00151`, `00170`, `00173`,
  `00179`, `00181`, `00200`, `00207`, `00208`, `00215`.
  Plain `exit=1` cases cover scalar boolean/unary, shifts/promotions,
  short/long compound arithmetic, string/null pointer comparison, integer to
  float conversion, double global comparisons, string literal indexing,
  conditional pointer/null expressions, and aggregate initializer checks.
  Crash cases include function-pointer/global-call paths (`00089`, `00170`),
  string/pointer/libc paths (`00173`, `00179`), global-array/control paths
  (`00181`), goto/VLA/conditional-control paths (`00207`, `00215`), and
  local/static struct initializer paths (`00208`). `00200` exits 27 with
  garbage type/promotion printf output from left-shift type checks.
- `RUNTIME_MISMATCH`, 15 tests:
  `00157`, `00159`, `00168`, `00169`, `00172`, `00174`, `00175`, `00176`,
  `00182`, `00185`, `00186`, `00193`, `00195`, `00196`, `00218`.
  Observed mismatches include local array indexing/loop values (`00157`),
  call argument and call-return corruption (`00159`, `00168`, `00196`),
  nested loop variable corruption (`00169`), pointer comparison results
  (`00172`), float/double arithmetic and conversion output (`00174`, `00175`,
  `00195`), quicksort/control-flow behavior (`00176`), initialized arrays and
  loop-counter corruption (`00185`), short loop/output truncation (`00186`),
  switch return/break behavior (`00193`), and unsigned enum bit-field
  zero/sign extension (`00218`).
  `00182.c` is explicitly now `RUNTIME_MISMATCH`: expected LED display output
  is replaced by a short bad-character output. Generated `00182.c.s` no longer
  shows the old illegal large-immediate `mov xN, #1234567` assembly form, so
  this does not reopen idea 307.
- `TIMEOUT`, 3 tests:
  `00143`, `00187`, `00220`.
  These remain timeout/output-storm candidates: Duff-device style switch/loop
  copy (`00143`), file I/O loops (`00187`), and wide-character/UTF-8 output
  (`00220`). No rerun was performed to separate true hang from output volume
  or stale-process effects.

Parked uncertain buckets preserved under this umbrella: runtime
nonzero/mismatch/crash, timeout/output-storm, semantic `lir_to_bir` admission
residuals, the `00189` externally binding symbol/PIC relocation residual, and
the three machine-printer/prepared-node singletons. Closed owners 285 through
307 should not be reopened from counts alone; `00182` only contradicts idea
307 if future generated-code evidence shows illegal large scalar immediates
again reaching AArch64 assembly.

## Suggested Next

Step 3 should not split an implementation owner from the broad runtime buckets
yet. The crispest focused owner candidate by current direct diagnostics is
`00189`: AArch64 externally binding data-symbol/PIC-safe address formation for
`stdout`-like dynamic symbols, with a narrow proof around
`c_testsuite_aarch64_backend_src_00189_c` plus any existing backend symbol
address tests. If the supervisor prefers a larger owner, the missing probe is
a narrow BIR/assembly comparison for the runtime mismatch/nonzero buckets to
prove a shared semantic cause rather than shared bad output.

## Watchouts

- Do not implement repairs under this umbrella idea.
- Do not reopen closed focused owners 285 through 307 from failure counts
  alone.
- Do not change expectations, allowlists, unsupported classifications,
  timeout policy, runner behavior, CTest registration, or proof-log policy.
- Treat `00140`, `00164`, and `00214` as distinct machine-printer/prepared-node
  diagnostics unless a later probe proves shared ownership.
- Treat `00204` and `00216` as parked `lir_to_bir` admission residuals until a
  probe shows one semantic lowering rule owns both.
- Keep `00182` in the runtime mismatch bucket unless generated assembly again
  shows illegal large scalar immediate materialization.

## Proof

Per packet instructions, no build or tests were run. Evidence used:
supervisor-created `test_before.log` from `cmake --build --preset default` and
`ctest --test-dir build -j10 -R backend --output-on-failure --timeout 30 >
test_before.log`, plus generated artifacts under
`build/c_testsuite_aarch64_backend/`.
