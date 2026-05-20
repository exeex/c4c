Status: Active
Source Idea Path: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize The Advanced HFA/Aggregate Output Mismatch

# Current Packet

## Just Finished

Step 1 localized the first `00204.c` mismatch after the byval lane repair to
caller-side AArch64 variadic HFA argument transport for the second long-double
HFA line:
`myprintf("%hfa33 %hfa34 %hfa34 %hfa34", hfa33, hfa34, hfa34, hfa34)`.

Exact first bad source value: the second variadic `hfa34` in that call should
print `34.1,34.4`, but the runtime prints `34.2,34.1`. The source arguments
lower to prepared BIR callsite `stdarg` block `entry` inst `491`:
`bir.call void myprintf(ptr @.str53, f128 %t86, f128 %t88, f128 %t90,
f128 %t94, f128 %t96, f128 %t98, f128 %t100, f128 %t104, f128 %t106,
f128 %t108, f128 %t110, f128 %t114, f128 %t116, f128 %t118, f128 %t120)`.
The prepared call metadata classifies it as `variadic_fpr_args=15` and assigns
the ABI lanes as:
`arg1..arg3 -> q0..q2` for `hfa33`, `arg4..arg7 -> q3..q6` for the first
`hfa34`, then incorrectly splits the second `hfa34` with
`arg8 -> q7`, `arg9 -> stack+0`, `arg10 -> stack+16`, `arg11 -> stack+32`.
The third `hfa34` starts at `stack+48`.

Generated assembly confirms the same split at
`build/c_testsuite_aarch64_backend/src/00204.c.s:9524`: `.str53` setup moves
`v0..v2` from the `hfa33`, loads `q3..q6` from the first `hfa34`, loads
`q7` from stack home `#9232`, then stores only the remaining lanes to outgoing
stack slots `0`, `16`, and `32`. It stores the next `hfa34` lane 0 at
outgoing `stack+48`.

The observing callee path is coherent with AAPCS64 whole-HFA overflow:
`myprintf` saves `q0..q7` in its FP register save area, initializes
`fp_offset` to `-128`, and the `%hfa34` `va_arg` path checks
`fp_offset + 64 > 0` before choosing the overflow path. After consuming
`hfa33` (`+48`) and one `hfa34` (`+64`), the second `hfa34` sees
`-16 + 64 > 0` and correctly reads the whole object from overflow memory.
Because the caller split the object, overflow slot 0 contains lane 1 (`34.2`)
and overflow slot 48 already contains the next `hfa34` lane 0 (`34.1`), which
explains the observed `34.2,34.1`.

Owner classification: first owner is HFA argument transport / call ABI lane
assignment for variadic AArch64 calls. It is not aggregate/floating return
publication, `va_arg` source selection/progression, HFA lane materialization,
stale homes, or output `printf` setup. Likely repair surfaces are the AArch64
call argument classifier/allocator in `src/backend/bir/lir_to_bir/call_abi.cpp`
and the downstream MIR call-boundary move planning that consumes prepared
callsite assignments. The semantic rule to preserve is that an HFA requiring
more FP/SIMD registers than remain must be assigned wholly to the stack; do not
split lanes between `q7` and overflow stack.

## Suggested Next

Step 2 should repair AArch64 variadic HFA call assignment so a same-HFA
aggregate is either fully in FP/SIMD registers or fully in the outgoing stack
area when the remaining FP/SIMD register count is insufficient. Start with the
`.str53` shape above as the narrow witness, then prove nearby same-feature
`hfa32`/`hfa33`/`hfa34`, double-HFA, and float-HFA cases are not harmed.

## Watchouts

Keep byval lane publication, fixed formal publication, return publication,
local/value homes, and frame/formal owners out of the Step 2 repair unless new
evidence moves the first bad fact back there. Do not downgrade expectations or
special-case `00204.c`; the same no-split HFA rule should cover the failing
long-double line and nearby variadic HFA mixtures. The long-double `va_arg`
path's overflow decision currently matches the ABI expectation and explains
the runtime symptom, so do not "fix" it to consume the caller's split layout.
Leave `review/326_stdarg_byval_route_review.md` untouched.

## Proof

Localization used focused source, prepared-BIR, callsite-metadata, assembly,
and current runtime-log probes only. Existing `test_after.log` contains the
representative failure:
`33.1,33.3 34.1,34.4 34.2,34.1 34.2,0.0` versus expected
`33.1,33.3 34.1,34.4 34.1,34.4 34.1,34.4`.
No full build was needed for this todo-only packet. Smallest credible Step 2
proof command after a code repair:
`ctest --test-dir build -j --output-on-failure -R 'backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication|c_testsuite_aarch64_backend_src_00204_c' | tee test_after.log`.
