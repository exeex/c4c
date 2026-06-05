Status: Active
Source Idea Path: ideas/open/112_aarch64_00216_00204_post_closure_regression.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Broader Validation And Closure Notes

# Current Packet

## Just Finished

Step 5 `Broader Validation And Closure Notes` ran the supervisor-selected
broader AArch64 validation after repairing the two backend-route byval
stack-overflow snippet contracts.

The original regressions are repaired in the broad run:
`c_testsuite_aarch64_backend_src_00216_c` and
`c_testsuite_aarch64_backend_src_00204_c` both pass.

Exact regression sources fixed by this active slice:
- direct frame-slot-backed local aggregate pointer operands can now find their
  explicit materialization when aggregate lane/base names such as `x.0` are
  used, repairing the `00216` call aggregate address path;
- outgoing AArch64 stack argument space is now reserved before `x16`-relative
  stores, with `x16` published from the adjusted `sp` and source operands based
  on the caller frame offset to remain correct after reservation;
- aggregate `va_arg` lowering now preserves and rematerializes the stable
  va-list object home established by `va_start`, repairing the remaining
  `00204` variadic HFA long-double aggregate path.

`00216` and `00204` belonged to the same call-aggregate/call-boundary
regression family, but they did not share one identical root cause. `00216`
was repaired by aggregate address materialization; `00204` additionally needed
the outgoing stack reservation order and callee-side variadic va-list lifetime
repair.

The two backend-route byval stack-overflow probes now assert the accepted
outgoing stack contract: `sub sp, sp, #32`, `mov x16, sp`, stores through
`x16`, call, then `add sp, sp, #32`. Their caller-frame source loads are
checked at the adjusted `sp` offsets after reservation.

## Suggested Next

Supervisor review/commit of the Step 5 closure-ready slice is the next
coherent packet.

## Watchouts

- Do not revert the accepted outgoing stack-reservation order in
  `calls.cpp`/`machine_printer.cpp`; the protected contract is `sub sp`,
  `mov x16, sp`, stores through `x16`, `bl`, then `add sp`.
- Do not weaken, skip, or reclassify `00216`, `00204`, or the idea-110
  recovered target tests.
- The byval stack-overflow route probes are expectation updates for the
  accepted stack-reservation contract, not coverage downgrades; they still
  require the concrete source loads, `x16` stores, call, and restore shape.

## Proof

Ran delegated proof:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -L aarch64 > test_after.log 2>&1
```

Result: build passed; CTest passed all 272 AArch64-labeled tests. Proof log
path: `test_after.log`.

Passing closure targets in this broader run included:
`c_testsuite_aarch64_backend_src_00216_c`,
`c_testsuite_aarch64_backend_src_00204_c`,
`c_testsuite_aarch64_backend_src_00172_c`,
`c_testsuite_aarch64_backend_src_00180_c`,
`c_testsuite_aarch64_backend_src_00220_c`,
`backend_codegen_route_aarch64_byval_payload_8_to_13_stack_overflow`, and
`backend_codegen_route_aarch64_byval_payload_9_to_14_stack_overflow`.
