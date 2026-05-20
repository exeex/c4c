Status: Active
Source Idea Path: ideas/open/339_aarch64_scalar_local_storage_writeback_sizing.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Scalar Local Slot Sizes And Writeback Path

# Current Packet

## Just Finished

Lifecycle switch from umbrella idea 295 to focused idea 339 is complete.

## Suggested Next

Execute Step 1 from `plan.md`: localize where the `00086` and `00111`
non-address-exposed scalar local cases lose usable slot size, initialization,
or writeback facts before AArch64 runtime.

## Watchouts

- Keep this owner focused on scalar local storage/writeback sizing for
  non-address-exposed scalar locals.
- Do not fold in parked FP comparison/expression, pointer/null conditional,
  broad return-spill, aggregate, variadic, timeout, runner, expectation, or
  CTest-registration work.
- Do not special-case `00086`, `00111`, stack offsets, variable names, or
  emitted instruction strings.

## Proof

Lifecycle-only switch. No implementation or validation command was run.
