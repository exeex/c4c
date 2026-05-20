Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Residual Failures

# Current Packet

## Just Finished

Step 2: Classify Residual Failures completed the post-338 backend-regex
classification from the existing Step 1 `test_after.log`, without
implementation, expectation, runner, registration, plan, idea, or proof-log
changes. The 27 residual failures are all external
`c_testsuite_aarch64_backend_*` entries; no local backend/unit/CLI tests are in
the failed list.

First-bad stage summary from `test_after.log`:

- Local backend/unit/CLI failures: none.
- Frontend/prepared-node, semantic `lir_to_bir`, assembler/linker legality:
  none visible in the failed list.
- AArch64 c-testsuite runtime nonzero/crash: 9 tests.
- AArch64 c-testsuite runtime mismatch: 14 tests.
- AArch64 c-testsuite timeout/output-storm: 4 tests.

Runtime nonzero/crash bucket:

- Scalar integer value lowering or ABI return-code corruption:
  `00086` exits 1 on short increment, `00111` exits 1 on short-minus-long,
  `00112` exits 240 on string pointer/null comparison, `00119` exits 16 on
  global double comparison, `00123` exits 160 on global double comparison,
  `00144` exits 64 on conditional null pointer conversions, and `00200`
  prints `0 test(s) failed` but exits 122 in the left-shift type test.
- Call/aggregate ABI crash: `00140` segfaults in a struct-by-value function
  with varargs-shaped calls.
- Control-flow/recursive global-array crash: `00181` segfaults in the Hanoi
  program.

Runtime mismatch bucket:

- Local/global array or aggregate address/value materialization:
  `00157` prints corrupt local array squares, `00176` prints corrupt global
  quicksort output after the initial array, `00195` reads zeroes from a global
  struct array of doubles, `00205` corrupts a large global aggregate
  initializer, and `00216` corrupts broad aggregate/compound-initializer
  layout output.
- Call result, enum, pointer, and scalar conversion residuals: `00159`
  corrupts `myfunc` return values, `00170` corrupts the final enum value,
  `00171` prints `b is NULL` after successfully dereferencing `b`, `00175`
  corrupts char/int/float conversions through calls and initializers, and
  `00218` reports `unsigned enum bit-fields broken`.
- Floating-point runtime value residuals: `00174` prints zeroes and corrupt
  comparison rows for float/double arithmetic, comparisons, assignment, casts,
  and `sin`.
- Control-flow/libc/string formatting residuals: `00182` prints blank LED
  rows, and `00186` prints only the first `sprintf` loop line.
- Variadic/ABI residual: `00204` is still a broad AArch64 calling-convention,
  byval/HFA, return, and stdarg runtime mismatch; it is not a single-count
  reopen candidate.

Timeout bucket:

- `00173` times out in string copy/pointer iteration code.
- `00187` times out in file I/O and character-reading loops.
- `00207` times out in a mixed VLA/goto/short-circuit-control test.
- `00220` times out in the wide-character UTF-8 fixture.

Closed-owner adjacency through idea 338:

- Adjacent to closed idea 338, but not inside its closed first-bad boundary by
  current evidence: `00173` is now timeout, `00175`, `00176`, `00204`,
  `00205`, and `00216` are runtime mismatches, and `00181` is a runtime
  segfault; the old scalar-cast machine-printer diagnostic is not the visible
  first bad fact.
- Adjacent to earlier closed return/call-result owner idea 336: `00159` and
  `00170` fail at runtime value output, but this log alone does not prove the
  same stale-return-register boundary that idea 336 closed.
- Adjacent to closed aggregate/address/materialization owners such as ideas
  298, 301, 305, 306, 307, 314, 315, and 316: `00157`, `00176`, `00182`,
  `00195`, `00204`, `00205`, and `00216` are runtime residuals or later
  mismatch surfaces, not proof that those closed compile/printer/assembler
  boundaries regressed.
- Adjacent to closed timeout/control/value owners such as ideas 280, 293, 296,
  299, and 301: `00173`, `00187`, `00207`, `00218`, and `00220` remain parked
  unless fresh generated-code or focused proof ties them to an exact closed
  boundary.
- Clearly new or currently under-evidenced residual families include the
  scalar integer/FP runtime-value group (`00086`, `00111`, `00112`, `00119`,
  `00123`, `00144`, `00174`, `00200`), pointer-null comparison `00171`,
  call/aggregate ABI crash `00140`, `sprintf`/loop output `00186`, and the
  timeout group. These need Step 3 evidence before selecting a focused owner.

## Suggested Next

Proceed to Step 3 only after the supervisor chooses whether to inspect
generated-code/proof evidence for one candidate bucket. No focused owner was
selected by this classification-only packet.

## Watchouts

- Do not implement under the umbrella inventory plan.
- Do not reopen closed owners through idea 338 from counts alone.
- Treat timeout and output-storm cases as unsafe for broad reruns unless the
  delegated command includes timeout and stale-process cleanup.
- Do not change expectations, runner behavior, unsupported classifications, or
  CTest registration to improve backend-regex counts.
- `ctest -j10` output is interleaved, so this packet used the final failed
  list, runner diagnostics, and shallow fixture inspection only; it did not
  claim generated-code ownership for any bucket.
- Timeout entries remain `src_00173_c`, `src_00187_c`, `src_00207_c`, and
  `src_00220_c`.

## Proof

Used the existing Step 1 proof log only; no broad tests were rerun for this
classification packet.

Existing Step 1 command that produced the proof log:

```sh
cmake --build build --target c4cll -j 2 && (ctest --test-dir build -j10 -R backend --output-on-failure > test_after.log 2>&1 || true)
```

Build reported `ninja: no work to do.` CTest wrote `test_after.log` and
reported `92% tests passed, 27 tests failed out of 354`; selected 354, passed
327, failed 23, timed out 4, skipped 0 visible in the log. The failed-list
surface contains only `c_testsuite_aarch64_backend_*` tests, not local backend
tests. Proof log path: `test_after.log`.
