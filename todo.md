Status: Active
Source Idea Path: ideas/open/382_aarch64_dynamic_stack_vla_fixed_slot_addressing.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Reconfirm Stage and First Bad Address

# Current Packet

## Just Finished

Lifecycle switched from closed idea 381 to active idea 382.

## Suggested Next

Execute Step 1 from `plan.md`: reconfirm `00207` is a generated-program
runtime timeout, verify fresh asm and binary emission complete before timeout,
and localize the first fixed stack home still addressed through the moved
`sp`.

## Watchouts

- Closed idea 381 owns `00200` shift/type-promotion backend asm-generation
  scalability and should stay closed unless fresh evidence invalidates its
  accepted repair.
- Do not edit tests, expectations, unsupported classifications, timeout policy,
  runners, CTest registration, proof-log policy, or c-testsuite sources.
- Do not special-case `00207`, `f1`, `boom!`, one label, one stack offset, one
  register, or one emitted instruction neighborhood.

## Proof

Close gate for idea 381:

- Regression guard script passed for canonical `test_before.log` /
  `test_after.log`: before 147 passed / 1 failed / 148 total; after 150 passed
  / 0 failed / 150 total; no new failures.
- Accepted full-suite baseline remains 3363 passed / 17 failed / 3380 total,
  no new failures, with only `00207` remaining as an AArch64 backend
  c-testsuite failure.
