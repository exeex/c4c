Status: Active
Source Idea Path: ideas/open/328_aarch64_byval_aggregate_call_argument_lane_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair General Small Byval Payload Publication

# Current Packet

## Just Finished

Step 2 repaired the general small byval aggregate payload publication path in
`src/backend/mir/aarch64/codegen/calls.cpp`.

Code owner: AArch64 call-boundary lowering. The stale path was not in memory
lowering or dispatch; the earlier register-source local-frame-address
publication and the later frame-slot call-argument move path could both treat a
register-passed integer byval aggregate as an address move unless the prepared
move already had `call_arg_byval_aggregate_register_lanes`.

Repair: register-passed integer `byval` call arguments with ABI size `1..16`
now use the aggregate register-lane publication record even when the incoming
prepared move is a generic call-argument move. The source is taken from
prepared byval lane stores when available, with the existing pointer-value
fallback preserved for register-backed aggregate storage. Larger indirect
byval arguments, stack-passed byval arguments, sret, and normal pointer
arguments stay on the existing address/copy paths.

Generated-code evidence for `fa_s1(s1)`: the callsite now stores the payload
byte at `[sp, #928]`, emits `ldrb w0, [sp, #928]`, then `bl fa_s1`. It no
longer forwards the prepared temporary address with `add x0, sp, #928`.

Nearby generated-code guard evidence remains in bounds: `fa_s2` still emits
byte loads plus `orr x0, x0, x9, lsl #8`; split-lane `fa_s10` still publishes
through `x0`/`x1`; and stack-passed `fa_s17` still passes its prepared object
address with `add x0, sp, #1064`.

## Suggested Next

Execute Step 3 from `plan.md`: add focused backend coverage for small
register-passed byval aggregate payload publication, including a guard that
distinguishes payload lane publication from forwarding the prepared object
address.

## Watchouts

- This packet intentionally did not touch tests; Step 3 should pin the general
  call-boundary contract outside `00204`.
- Keep coverage semantic: no `00204`, `fa_s1`, argument-name, stack-offset, or
  exact scratch-register matching.
- The new helper is ABI-shape based: pointer byval, passed in register, integer
  class, size `1..16`. If later evidence finds a non-integer or HFA byval case,
  that is a separate owner.

## Proof

Ran the delegated proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^c_testsuite_aarch64_backend_src_00204_c$' | tee test_after.log`

Result: build succeeded and `c_testsuite_aarch64_backend_src_00204_c` passed,
1/1. Proof log preserved at `test_after.log`.
