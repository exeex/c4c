Status: Active
Source Idea Path: ideas/open/373_aarch64_static_global_selected_value_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Selected Value Publication Gap

# Current Packet

## Just Finished

Executed Step 1 localization for `c_testsuite_aarch64_backend_src_00182_c`.
The focused proof still fails with `RUNTIME_MISMATCH`: expected LED rendering
for `1234567`, actual output renders every digit as the zero pattern.

Prepared BIR for `print_led` is semantically correct. The digit extraction path
stores `x % 10` through selected static lanes, and the top-line read has:
`%t33 = load_local %lv.i`, `%t34 = sext %t33`, static lane loads
`%t36.outer0.eltN`, then `%t36.outer0 = select eq i64 %t34, N, ...`, followed
by `bir.call void topline(i32 %t36.outer0, ptr %t37)`. The prepared callsite
expects arg0 from `%t36.outer0` in `x21` and arg1 from `%t37` in `x13`.

First bad generated AArch64 fact is in the static selected read before the
first `topline` call. In `.LBB195_113`, the loop index is loaded and extended
into `x13`, but lane snapshotting immediately overwrites `w20` with `d[0]`,
`w21` with `d[1]`, and later overwrites `x13` with the buffer pointer. The
select materialization then compares `mov w9, w20; sxtw x9, w9; cmp x9,#N`
instead of comparing the preserved `%t34` index. It also leaves the selected
value in scratch `w9` before `mov w0,w9`, rather than publishing the prepared
`%t36.outer0` carrier/home (`x21`) that the call-boundary plan names.

Owner boundary: AArch64 selected scalar value materialization/publication for
static/global selected aggregate reads feeding scalar consumers. The generated
code does not protect the selector carrier from lane loads/consumer-argument
materialization and does not consistently publish the selected result to its
prepared carrier before the call consumer.

Rejected adjacent owners based on evidence: this is not the unsigned remainder
or digit extraction lowering, not static/global selected store writeback, not
the `topline`/`midline`/`botline` callees, and not a generic call ABI argument
move bug. Older closed `00182` buckets should remain archived unless Step 2
proves an exact boundary overlap.

## Suggested Next

Execute Step 2: add focused backend coverage for a selected static/global
scalar value loaded by dynamic index and consumed by a scalar call/control path.
The test should prove that the selector carrier remains distinct from lane
value carriers and later call/pointer argument materialization, and that the
final selected value is published to the prepared carrier/home before the
consumer observes it. Keep it semantic: avoid `00182`, `print_led`, one static
symbol, 32 lanes, one register, or one exact label sequence.

## Watchouts

Do not reopen closed `00182` owners from counts alone. Do not special-case
`00182`, `print_led`, one digit array, one call target, one register, one
stack offset, or one emitted instruction sequence. Keep scalar constant-binary
stack publication (`00205` first bad fact), external call-result publication
(`00187`), scalar FP (`00174`), timeouts (`00200`, `00207`), and bit-field
local aggregate address publication (`00218`) parked under idea 295 unless
fresh localization proves a handoff.

## Proof

Ran exact Step 1 command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^c_testsuite_aarch64_backend_src_00182_c$' > test_after.log 2>&1
```

Result: build completed, selected 1 test, 0 passed, 1 failed
(`c_testsuite_aarch64_backend_src_00182_c`) with the runtime mismatch above.
Inspection used generated
`build/c_testsuite_aarch64_backend/src/00182.c.s` plus
`build/c4cll --dump-prepared-bir --mir-focus-function print_led
tests/c/external/c-testsuite/src/00182.c`. A `--trace-mir --target
aarch64-linux-gnu` probe was not useful because the current trace route reports
the x86/debug owner.
