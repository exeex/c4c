Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Capture Or Reconstruct Post-302 Backend Inventory

# Current Packet

## Just Finished

Step 1: Capture Or Reconstruct Post-302 Backend Inventory completed from the
supervisor-captured broad backend baseline in `test_before.log`.

Inventory scope:

- Command represented by the log: `(cd build && timeout 900 ctest -j10 -R backend --output-on-failure)`.
- CTest selected 352 backend-matching tests.
- Result: 305 passed, 47 failed.
- Local backend/unit coverage passed: all 140 non-`c_testsuite_aarch64_backend_*`
  selected tests passed; all 47 failures are residual
  `c_testsuite_aarch64_backend_*` tests.
- AArch64 c-testsuite backend coverage selected 212 tests: 165 passed, 44
  failed, and 3 timed out.

Residual `c_testsuite_aarch64_backend_*` failures by mechanism:

- `BACKEND_FAIL` assembly/link failure, 4 tests:
  `00050.c`, `00176.c`, `00182.c`, `00189.c`.
  `00050.c`, `00176.c`, and `00182.c` show invalid AArch64 `adrp wN,
  symbol+offset` operands. `00189.c` reaches link failure on
  `R_AARCH64_ADR_PREL_PG_HI21` against externally binding `stdout`.
- `FRONTEND_FAIL` semantic/printer gate failure, 5 tests:
  `00140.c`, `00164.c`, `00204.c`, `00214.c`, `00216.c`.
  Sub-buckets: unsupported call-boundary move printer node (`00140.c`),
  incomplete scalar multiply printable rhs facts (`00164.c`), memory operand
  from unprepared frame-slot source (`00214.c`), and semantic `lir_to_bir`
  outside admitted backend capability buckets (`00204.c`, `00216.c`).
- `RUNTIME_NONZERO`, 22 tests:
  `00035.c`, `00066.c`, `00086.c`, `00089.c`, `00102.c`, `00111.c`,
  `00112.c`, `00113.c`, `00119.c`, `00123.c`, `00137.c`, `00138.c`,
  `00144.c`, `00151.c`, `00170.c`, `00173.c`, `00179.c`, `00181.c`,
  `00200.c`, `00207.c`, `00208.c`, `00215.c`.
  Exit sub-buckets: plain `exit=1` for 14 tests, `Bus error` for `00089.c`,
  `Segmentation fault` for `00170.c`, `00173.c`, `00179.c`, `00181.c`,
  `00207.c`, `00208.c`, and `00215.c`, and `exit=27` for `00200.c`.
- `RUNTIME_MISMATCH`, 13 tests:
  `00157.c`, `00159.c`, `00168.c`, `00169.c`, `00172.c`, `00174.c`,
  `00175.c`, `00185.c`, `00186.c`, `00193.c`, `00195.c`, `00196.c`,
  `00218.c`.
- CTest timeout, 3 tests:
  `00143.c`, `00187.c`, `00220.c`.

## Suggested Next

Use Step 2 to choose the next focused owner from one mechanism bucket, with a
preference for a semantic bucket that can be proved by nearby tests rather than
a named-case-only repair. The clearest candidates from this inventory are the
`adrp wN, symbol+offset` invalid-operand cluster, the semantic `lir_to_bir`
capability gate cluster, or the runtime segfault/nonzero cluster after a
focused evidence pass.

## Watchouts

- This umbrella is classification-only; do not delegate implementation until a
  focused semantic owner is split.
- Do not reopen closed owners 285 through 305 from failure counts alone.
- Separate local backend/unit failures from `c_testsuite_aarch64_backend_*`
  failures.
- Broad runtime scans need timeout and stale-process cleanup before their logs
  are trusted.
- Do not edit implementation files, closed ideas, test logs, expectations,
  allowlists, unsupported classifications, timeout policy, runner behavior, or
  CTest registration.
- Evidence gaps for Step 2: `test_before.log` identifies harness mechanisms
  and enough stderr snippets to bucket failures, but it does not contain a
  focused BIR/MIR/assembly trace for each runtime mismatch/nonzero. Runtime
  stdout/stderr is empty for most nonzero cases, so Step 2 should gather
  targeted per-bucket evidence before assigning implementation.

## Proof

No tests were rerun per packet instruction. Inventory proof used the existing
supervisor-captured `test_before.log` from `(cd build && timeout 900 ctest -j10
-R backend --output-on-failure)`, which selected 352 tests and ended with 47
failures. `test_after.log` was not modified.
