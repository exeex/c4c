Status: Active
Source Idea Path: ideas/open/328_aarch64_byval_aggregate_call_argument_lane_publication.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Representative Progress

# Current Packet

## Just Finished

Step 4 proved representative `00204` progress after the byval payload-lane
repair.

Fresh generated output for `fa_s1(s1)` no longer shows the targeted byval
temporary-address call argument fault. The callsite stores the prepared source
byte to the byval temp and loads the payload into the ABI argument lane before
the call:

```asm
ldrb w13, [x9]
strb w13, [sp, #928]
ldrb w0, [sp, #928]
bl fa_s1
```

The old targeted bad fact, `add x0, sp, #928; bl fa_s1`, is absent at the
representative `fa_s1` callsite. The representative test passed, so no next
distinct first bad fact remains for `00204` in this packet.

## Suggested Next

Execute Step 5 from `plan.md`: run the supervisor-selected adjacent byval and
variadic guard subset, then record whether prior byval placement, upper-lane
publication, fixed-formal entry, and local/value-home guardrails remain stable.

## Watchouts

- This packet only proved the representative `00204` target selected by the
  supervisor. Broader adjacent guard selection remains supervisor-owned.
- Larger stack-passed byval behavior still legitimately materializes an
  address, for example `add x0, sp, #1064; bl fa_s17`; that is not the repaired
  small integer-class register-passed byval lane fault.
- No expectation, allowlist, CTest registration, runner, timeout, proof-log,
  implementation, or test files were changed.

## Proof

Ran the delegated proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^c_testsuite_aarch64_backend_src_00204_c$' | tee test_after.log`

Result: build was up to date and `c_testsuite_aarch64_backend_src_00204_c`
passed, 1/1. Proof log preserved at `test_after.log`.
