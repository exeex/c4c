Status: Active
Source Idea Path: ideas/open/330_aarch64_non_hfa_aggregate_va_arg_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Non-HFA Aggregate Va Arg Materialization

# Current Packet

## Just Finished

Lifecycle switched from idea 329 after Step 4 classified the remaining
`00204.c` mismatch as a distinct non-HFA aggregate `va_arg` materialization
owner. Idea 329 call-operand publication is inactive: generated `myprintf`
now publishes `.str31` / `.str33` into `x0` and the aggregate buffer pointer
into `x1` before `bl printf`.

## Suggested Next

Delegate Step 1 localization for idea 330. Trace the `%7s` and `%9s`
branches from non-HFA aggregate `va_arg` source selection through the
destination stack buffers (`sp + 8` and `sp + 15`) and identify why selected
aggregate bytes are not copied before the following `printf` observes them.

## Watchouts

Do not reopen idea 329 call-operand publication unless generated code again
reaches a call without the fixed format string in `x0` and aggregate-derived
operand in the assigned ABI register. Do not pull idea 326 HFA/floating
residuals into this route until non-HFA aggregate string materialization is
cleared and fresh evidence reaches those paths.

## Proof

Lifecycle-only transition. Existing focused proof in `test_after.log` shows
11 of 12 delegated tests passed, with only
`c_testsuite_aarch64_backend_src_00204_c` failing as a classified
non-HFA aggregate `va_arg` materialization residual.
