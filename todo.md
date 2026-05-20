Status: Active
Source Idea Path: ideas/open/328_aarch64_byval_aggregate_call_argument_lane_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Repair Partial Upper-Lane Byval Publication

# Current Packet

## Just Finished

Continued plan Step 1 by refining AArch64 byval aggregate lane publication so
fragmented partial-lane publication is gated to late temp homes at or after the
aggregate source home. The previously regressed all-byval `stdarg:` payload and
the following `lmnopqr` payload now both print `ABCDEFGHI` for the byval
arguments in `00204.c`.

## Suggested Next

Stop this packet. If the supervisor keeps this route, the next packet should
investigate the advanced first bad fact in the following HFA/aggregate output
section.

## Watchouts

Keep the repair semantic for AArch64 byval aggregate lane publication. Do not
key it to `00204.c`, `%9s`, `stdarg`, `myprintf`, `s9`, one stack slot, or one
emitted instruction sequence. The byval lane publication first bad fact has
advanced; do not broaden this patch into HFA/FP aggregate output unless the
supervisor delegates that route. Leave `review/326_stdarg_byval_route_review.md`
untouched.

## Proof

Ran `ctest --test-dir build --output-on-failure -R '^(backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication|backend_runtime_byval_helper_payload_8_to_13|backend_runtime_byval_helper_payload_9_to_14|c_testsuite_aarch64_backend_src_00204_c)$' > test_after.log 2>&1`; 3/4 tests passed and `c_testsuite_aarch64_backend_src_00204_c` failed. First remaining mismatch after the fixed byval payloads: expected `33.1,33.3 34.1,34.4 34.1,34.4 34.1,34.4`, actual `33.1,33.3 34.1,34.4 34.2,34.1 34.2,0.0`. Ran `git diff --check`; it passed. Proof log: `test_after.log`.

Supervisor broader validation passed:
`ctest --test-dir build -j --output-on-failure -R '^backend_'`
passed 140/140.
