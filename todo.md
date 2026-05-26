Status: Active
Source Idea Path: ideas/open/08_calls_argument_sources_retirement.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Move Or Redirect Remaining Source Choice To Prepared Facts

# Current Packet

## Just Finished

Step 2 retired the absent-selection frame-slot address recovery path from
`make_frame_slot_call_argument_address_source`.

The AArch64 frame-slot address helper now fails closed when
`PreparedCallArgumentPlan::source_selection` is absent and only consumes
complete prepared `FrameSlotAddress` selections. It no longer scans prepared
addressing materializations, source homes, local aggregate stack objects, or
frame slots to recover stack-homed local aggregate address sources
target-locally.

Prepared call planning now owns the moved source choice. For stack-homed pointer
arguments it selects `FrameSlotAddress` from the latest prepared frame-slot
materialization when present, or from the prepared local aggregate object/slot
facts when the address has no materialization record. The missing local
aggregate/publication path preserves the old source-home size/align behavior;
ordinary local aggregate address publication uses the selected stack object
size/align. AArch64 dispatch now consumes explicit `FrameSlotAddress` selections
in the missing-frame-slot materialization bridge instead of relying on helper
recovery.

Focused tests cover the moved authority: prepared call-plan tests assert the
stack-homed missing-local-aggregate address source fact, and AArch64 dispatch
fixtures carry explicit `FrameSlotAddress` facts while asserting that absent
selection no longer rederives the object address in AArch64.

## Suggested Next

Continue Step 2/Step 3 by auditing the now-selection-only address helpers and
shrinking the exported AArch64 calls helper surface where callers can consume
prepared `source_selection` directly.

## Watchouts

- Keep semantic source-choice authority in prepared call facts, not AArch64
  helper reshuffles.
- Do not edit `ideas/closed/`, `review/`, or non-canonical test logs as part of
  this execution route.
- Treat expectation weakening, duplicate prepared/AArch64 source selection, and
  helper-only moves as route failures.
- Do not fold `find_frame_slot_by_id` into call-specific code; it is a shared
  lookup used by byval aggregate and cast lowering.
- The stack-homed address rule has two prepared size/align cases: ordinary local
  address publication uses the stack object, while the missing-publication path
  uses the source home. Keep both covered when shrinking helpers.

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
