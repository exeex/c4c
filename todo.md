Status: Active
Source Idea Path: ideas/open/373_aarch64_static_global_selected_value_publication.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Representative And Classify Adjacency

# Current Packet

## Just Finished

Executed idea 373 Steps 2 and 3. Added focused backend coverage for a dynamic
selected global scalar read feeding a scalar call argument while lane registers
and a later pointer argument use overlapping prepared carriers. The focused
test asserts that the dynamic selector is not replaced by a selected lane
carrier and that the selected i32 value reaches the prepared call-source
register before the call.

Repair: AArch64 direct global select-chain call-argument materialization now
targets the selected value's prepared GPR home when one exists, chooses a
distinct scratch register, records the prepared value id/name for that carrier,
and reloads current memory-load/cast operands at the producer boundary instead
of trusting stale register homes from after later lane snapshots. The cast
publication helper now recursively materializes cast operands at the cast
producer index, not at the later consumer index, so a widened selector cannot
reuse a register home after intervening selected lane loads have clobbered it.

Generated `00182.c.s` now advances through the localized gap and passes. In
`print_led`, the first selected static digit read reloads the loop index from
`[sp,#16]` into the prepared selected-result carrier, extends it, and compares
that value before selecting lanes: `ldr w21, [sp,#16]; sxtw x21,w21; cmp
x21,#0 ...`. True arms load selected static lanes into `w21`, and the call
uses the selected value from `w21` rather than stale lane `w20` or scratch `w9`.

## Suggested Next

Execute Step 4 narrow proof/classification or supervisor regression guard. The
representative and protected regressions are green in the delegated subset; any
broader residuals should remain under the inventory umbrella unless a new first
bad fact is found.

## Watchouts

Do not reopen closed `00182` owners from counts alone. Do not special-case
`00182`, `print_led`, one digit array, one call target, one register, one
stack offset, or one emitted instruction sequence. Keep scalar constant-binary
stack publication (`00205` first bad fact), external call-result publication
(`00187`), scalar FP (`00174`), timeouts (`00200`, `00207`), and bit-field
local aggregate address publication (`00218`) parked under idea 295 unless
fresh localization proves a handoff.

## Proof

Ran exact Steps 2/3 proof command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_.*|c_testsuite_aarch64_backend_src_00182_c|c_testsuite_aarch64_backend_src_00163_c|c_testsuite_aarch64_backend_src_00151_c|c_testsuite_aarch64_backend_src_00214_c)$' > test_after.log 2>&1
```

Result: passed. `test_after.log` reports 147 selected tests, 147 passed, 0
failed. This includes `backend_.*`, `00182`, and protected representatives
`00163`, `00151`, and `00214`.

Supervisor full-suite candidate against the accepted baseline also passed the
monotonic guard: `3351` passed / `24` failed before, `3352` passed / `23`
failed after, resolving `c_testsuite_aarch64_backend_src_00182_c` with no new
failing tests.
