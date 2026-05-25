Status: Active
Source Idea Path: ideas/open/15_aarch64_aggregate_hfa_stdarg_abi.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Reproduce And Localize The 00204 ABI Failure

# Current Packet

## Just Finished

Step 1 reproduced `c_testsuite_aarch64_backend_src_00204_c` with the delegated
proof and localized the first bad owner. The first mismatch occurs in the
ordinary `Arguments:` section at `fa_s1(s1)`, before stdarg; HFA fixed
arguments and all return-value checks are still correct. `fa_s1` expects the
small non-HFA aggregate payload in `x0` and unpacks it byte-wise, but `arg`
stores `s1` into a local frame byte at stack offset 928 and branches to
`fa_s1` without publishing that byte to `x0`. For `fa_s2`, prepared metadata
records `arg0 bank=gpr from=frame_slot:stack+1224 to=x0`, but the actual
aggregate bytes were just stored at stack offsets 929 and 930. The concrete
suspect is the AArch64 byval aggregate register-lane call-argument source path:
`call_arg_byval_aggregate_register_lanes` is sourcing the aggregate value home
or prior-preservation spill instead of the byte-bearing aggregate frame slots
created by the immediately preceding stores.

## Suggested Next

Add a focused reduced test for a 1-2 byte non-HFA struct passed by value on
AArch64, then repair the byval register-lane source selection so the call move
loads from the populated aggregate bytes before publishing `x0`/`x1`.

## Watchouts

Do not reopen broad prior-preservation fallback. The failing prepared facts are
visible around `src/backend/prealloc/call_plans.cpp` source selection and the
AArch64 consumers in
`src/backend/mir/aarch64/codegen/calls_byval_aggregates.cpp` /
`src/backend/mir/aarch64/codegen/calls_moves.cpp`. The callee prologue path for
small byval aggregates appears ABI-compatible for this first failure, and HFA
fixed-argument paths are not the first bad owner.

## Proof

Ran:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^c_testsuite_aarch64_backend_src_00204_c$') > test_after.log 2>&1`

Result: build succeeded; the focused ctest failed with the reproduced
`[RUNTIME_MISMATCH]`. Proof log: `test_after.log`.
