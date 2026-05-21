Status: Active
Source Idea Path: ideas/open/370_aarch64_short_circuit_control_publication_regression.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Baseline Regression Is Removed

你該做baseline sanity check了

# Current Packet

## Just Finished

Step 4 narrow proof/classification completed against implementation baseline
`ea9d30f56 Repair AArch64 short-circuit join publication`.

`c_testsuite_aarch64_backend_src_00196_c` now passes in the delegated narrow
proof, so the baseline regression introduced after idea 366 is removed for the
representative case. `c_testsuite_aarch64_backend_src_00112_c` also remains
passing, preserving the string-literal pointer/null comparison repair.

Supervisor full-suite regression guard also passed against the accepted
baseline. `test_baseline.log` before the repair had `3346` passed, `29`
failed, `3375` total; the fresh full-suite `test_after.log` has `3347` passed,
`28` failed, `3375` total. The guard reports one resolved failing test,
`c_testsuite_aarch64_backend_src_00112_c`, and no new failing tests. The
accepted baseline log was rolled forward to this full-suite result.

Assembly classification for
`build/c_testsuite_aarch64_backend/src/00196.c.s` confirms the repaired
boundary:

```asm
bl fred
mov x13, x0
cmp w13, #0
b.ne .LBB91_18
b .LBB91_19
.LBB91_18:
bl joe
mov x21, x0
b .LBB91_20
.LBB91_20:
mov w13, #1
mov w9, w21
add x13, x13, x9
cmp w13, #0
cset w13, ne
b .LBB91_21
.LBB91_19:
mov x13, #0
b .LBB91_21
.LBB91_21:
adrp x0, .str2
add x0, x0, :lo12:.str2
mov x0, x20
mov x1, x13
bl printf
```

The RHS-derived `(1 + joe()) != 0` add/compare is on the RHS edge before the
join. The skip edge publishes `x13 = 0`. The join no longer recomputes
RHS-derived add/compare from stale `%t55/w21`; it consumes the prepared edge
publication in `x13`.

## Suggested Next

Supervisor can route idea 370 to lifecycle closure.

## Watchouts

This packet was proof/classification only. No implementation or focused-test
files were modified. The narrow proof does not replace the supervisor-owned
full-suite regression guard against `test_baseline.log`.

## Proof

Ran the delegated Step 4 narrow proof:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(c_testsuite_aarch64_backend_src_00196_c|c_testsuite_aarch64_backend_src_00112_c)$' > test_after.log 2>&1
```

Result: passed, `100% tests passed, 0 tests failed out of 2`.
`c_testsuite_aarch64_backend_src_00196_c` and
`c_testsuite_aarch64_backend_src_00112_c` are green. `test_after.log` is the
preserved proof log.

Ran the supervisor full-suite guard:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure > test_after.log 2>&1
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_baseline.log --after test_after.log
```

Result: passed. The comparison reports `3346 -> 3347` passed, `29 -> 28`
failed, resolved `c_testsuite_aarch64_backend_src_00112_c`, and no new failing
tests.
