# AArch64 C-Testsuite Failure Family Inventory Todo

Status: Active
Source Idea Path: ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Sample Remaining Failure Families

# Current Packet

## Just Finished

Completed Step 3 by sampling representatives from the refreshed AArch64
backend classification. No implementation files, tests, lifecycle source
files, classifications, allowlists, CTest configuration, or root proof logs
were changed.

Sampling inputs:

- Refreshed classification:
  `/tmp/c4c_aarch64_backend_scan_212_refresh.classified`
- Focused representative proof:
  `/tmp/c4c_aarch64_step3_representatives.ctest.log`
- Representative subset:
  `src/00009.c`, `src/00012.c`, `src/00019.c`, `src/00056.c`,
  `src/00089.c`, `src/00124.c`, `src/00132.c`, `src/00156.c`,
  `src/00161.c`, `src/00170.c`, `src/00173.c`, `src/00211.c`,
  `src/00220.c`

Representative sampling results by bucket:

- Runtime scalar expression/value authority:
  - `src/00009.c` should reduce ordinary `*`, `/`, `%`, and subtraction to
    return 0, but assembly loads from the local slot and stores stale `w19`
    instead of publishing each arithmetic result; focused rerun exited 6.
  - `src/00012.c` should return constant 0, but assembly emits
    `sub w0, w19, #8`; focused rerun exited 112.
  - `src/00056.c` prints `12, 0` instead of `12, 34`; assembly passes only the
    first loaded local as a vararg and leaves the second scalar argument
    unmaterialized.
  - `src/00211.c` prints a random-looking value for `n+1`; assembly passes the
    format pointer but never materializes the integer argument.
- Runtime control-flow condition/state authority:
  - `src/00156.c` expects a `1..10` loop but prints nothing; assembly compares
    the induction variable against `0` and exits immediately.
  - `src/00161.c` expects a Fibonacci-style loop but prints only `1`; assembly
    loses the `a = t + p` update and compares stale state against `0`.
  - Nearby mismatch samples such as `src/00183.c`, `src/00192.c`,
    `src/00193.c`, and `src/00194.c` show the same broader symptom: branch
    predicates and selected expression values are emitted from stale or
    placeholder registers instead of the semantic scalar value.
- Runtime pointer/aggregate address authority:
  - `src/00019.c` segfaults; assembly stores uninitialized `x13` instead of
    `&s` for `s.p`, then dereferences through it.
  - `src/00172.c`-style pointer/local samples show address slots and equality
    arguments being read from unset stack locations.
  - This is likely a later owner; it is related to scalar value publication but
    broad enough to keep separate from the first scalar split.
- Closed-owner consistency checks:
  - `src/00089.c` still segfaults, but the sampled assembly shows `go()` and
    `anon()` return callable addresses; the remaining bad edge is a stale
    aggregate pointer load (`ldr x13, [x19]`) before the final indirect call,
    not the closed function-pointer callee materialization owner.
  - `src/00124.c` still fails nonzero in the focused rerun, but `f1` returns
    `f2` and the caller performs `blr`; the remaining issue is stale scalar
    call argument/result authority around the second indirect call, not a
    reason to reopen the closed function-pointer owner from counts alone.
- Timeout/hang bucket:
  - `src/00132.c`, `src/00173.c`, and `src/00220.c` still timeout under a
    5-second focused rerun.
  - `src/00132.c` generated a very large timeout run-output artifact in the
    build tree; no generated-runtime process was left running afterward.
  - The timeout cases are not ready as the next owner because they mix loop
    condition loss, string/pointer stepping, and wide-character data handling.
- Compile-stage/frontend-classified bucket:
  - Sampled `FRONTEND_FAIL` representatives are printer/lowering admission
    gaps, not runtime semantic owners for this split: `src/00024.c` fails on
    scalar `sub` with immediate lhs/register rhs, `src/00027.c` on scalar
    `or`, `src/00126.c` on scalar `xor`, and `src/00212.c` on fused
    compare-branch operands.

Rejected or deferred candidate owners:

- Reopen SP/frame alignment: rejected. The closed bus-error owner remains
  separated; sampled current failures are stale values or pointer contents,
  not misaligned `sp` evidence.
- Reopen direct string/global address or prepared call-argument authority:
  rejected. Current stdio mismatches such as `src/00056.c` and `src/00211.c`
  now show missing scalar varargs, not missing format/string pointer setup.
- Reopen function-pointer indirect-call values: rejected. `src/00089.c` and
  `src/00124.c` still fail, but the sampled call edges preserve function
  addresses; the bad values are aggregate/scalar inputs around those calls.
- Select timeout/hang owner: deferred. The timeout cases are real but unsafe as
  the next split without a smaller timeout-specific probe.
- Select compile-stage printer owner: deferred. The frontend-classified bucket
  is coherent, but it is separate from the larger remaining runtime semantic
  failures.

Selected next semantic owner:

- Split a focused AArch64 scalar expression/control-value authority owner.
  The first repair idea should cover publishing selected scalar expression
  results, constants, local loads/stores, branch predicates, return values, and
  scalar call arguments from authoritative semantic values instead of stale
  scratch registers. Starter representatives should be `src/00009.c`,
  `src/00012.c`, `src/00056.c`, `src/00156.c`, `src/00161.c`, and
  `src/00211.c`, with pointer/aggregate, timeout, and compile-stage printer
  cases explicitly out of scope.

## Suggested Next

Ask the plan owner to run Step 4 and create one focused source idea for the
selected AArch64 scalar expression/control-value authority owner. Keep the
starter scope to the scalar runtime representatives above and explicitly defer
pointer/aggregate addressing, timeout loops, and compile-stage printer gaps.

## Watchouts

- This umbrella route is for inventory and splitting only; do not implement a
  repair directly from it.
- The next idea should avoid a testcase-shaped contract. The owner is scalar
  value authority across expression results, predicates, returns, and scalar
  call arguments, not a named fix for `00009`, `00012`, or `00211`.
- Keep `src/00089.c` and `src/00124.c` in the evidence notes as closed-owner
  consistency checks, but do not reopen their closed function-pointer owners
  without more contradictory proof.
- Keep `src/00019.c` and pointer/aggregate address failures as likely follow-on
  work unless the scalar owner naturally proves a shared value-publication
  primitive.
- Keep timeout cases quarantined. After the focused rerun,
  `pgrep -af '^/workspaces/c4c/build-aarch64-scan/c_testsuite_aarch64_backend/'`
  found no generated-runtime processes, but `src/00132.c` left a very large
  generated run-output artifact in the build tree.
- Keep compile-stage printer failures out of the scalar runtime owner even
  though they are backend printer/lowering gaps.

## Proof

Inventory-only proof used the refreshed classification plus a timeout-safe
focused representative rerun:

```bash
ctest --test-dir build-aarch64-scan -R 'c_testsuite_aarch64_backend_src_(00009|00012|00019|00056|00089|00124|00132|00156|00161|00170|00173|00211|00220)_c$' -j 4 --timeout 5 --output-on-failure > /tmp/c4c_aarch64_step3_representatives.ctest.log 2>&1
pgrep -af '^/workspaces/c4c/build-aarch64-scan/c_testsuite_aarch64_backend/' || true
```

The representative rerun failed as expected for inventory sampling and
reproduced the target buckets: runtime nonzero, runtime mismatch, and timeout.
No broad scan was run for this packet, no root proof logs were used, and no
generated-runtime process remained after the focused rerun.
