Status: Active
Source Idea Path: ideas/open/08_calls_argument_sources_retirement.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Move Or Redirect Remaining Source Choice To Prepared Facts

# Current Packet

## Just Finished

Step 2 retired the absent-selection frame-slot value source recovery path from
`make_frame_slot_call_argument_source`.

The AArch64 frame-slot value helper now fails closed when
`PreparedCallArgumentPlan::source_selection` is absent and only consumes
complete prepared `FrameSlotValue` selections. It no longer scans prepared
addressing accesses, source homes, or frame slots to recover source bytes
target-locally.

Focused dispatch fixtures that still need frame-slot source bytes now carry the
prepared source facts explicitly: `FrameSlotValue` for ordinary frame-slot,
f128, va-arg-copy, and stack-preserved pointer arguments, and
`ByvalRegisterLane` for the stack-backed byval lane case. The absent-selection
value/byval/indirect-byval fixtures now assert `MissingValueAuthority` instead
of accepting legacy frame-slot recovery.

## Suggested Next

Continue Step 2 with the remaining absent-selection frame-slot address
compatibility path in `make_frame_slot_call_argument_address_source`, especially
the missing-local-aggregate object/slot search for stack-homed arguments.

## Watchouts

- Keep semantic source-choice authority in prepared call facts, not AArch64
  helper reshuffles.
- Do not edit `ideas/closed/`, `review/`, or non-canonical test logs as part of
  this execution route.
- Treat expectation weakening, duplicate prepared/AArch64 source selection, and
  helper-only moves as route failures.
- Do not fold `find_frame_slot_by_id` into call-specific code; it is a shared
  lookup used by byval aggregate and cast lowering.
- `make_frame_slot_call_argument_address_source` still owns the separate
  missing-local-aggregate frame-slot address compatibility path for stack-homed
  arguments; this slice only retired the frame-slot value source fallback.

## Proof

Passed; proof log preserved at `test_after.log`.

Delegated command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00204_c|c_testsuite_aarch64_backend_src_00216_c)$'
```

Result: build succeeded and all four delegated tests passed:
`backend_prepare_frame_stack_call_contract`,
`backend_aarch64_instruction_dispatch`,
`c_testsuite_aarch64_backend_src_00204_c`, and
`c_testsuite_aarch64_backend_src_00216_c`.
