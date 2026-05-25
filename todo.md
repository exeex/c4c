Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Retire One Proven Duplicate Helper Boundary

# Current Packet

## Just Finished

Step 2 of `plan.md` retired the explicit
`PreparedCallArgumentSourceSelectionKind::FrameSlotValue` selected-source
fallback boundary for AArch64 frame-slot stack call arguments. Explicit
`FrameSlotValue` selections now require a complete prepared payload including
source identity, stack slot, offset, size, alignment, and stack-slot home kind;
the helper no longer borrows the legacy `source_home` identity to complete an
explicit selection.

The instruction-dispatch coverage now includes an incomplete explicit
`FrameSlotValue` selection while matching legacy value-home facts are still
present; `lower_before_call_moves` fails closed instead of rederiving the
source from the legacy home. A paired absent-selection case proves the retained
no-`source_selection` compatibility path still lowers from the legacy
frame-slot home.

## Suggested Next

Continue Step 2 by auditing the remaining selected-source helper surfaces for a
small complete-payload-only boundary that can be retired without touching
absent-selection compatibility. A good next packet is to look at the remaining
register-destination frame-slot value and aggregate/byval-adjacent call
argument paths that still probe helper results before choosing legacy fallback
behavior.

## Watchouts

- Do not retire fragmented byval lane fallback logic outside
  `make_byval_register_lane_prepared_source`; the accepted prerequisite only
  covers complete byval register-lane selections with prepared source payload.
- `make_frame_slot_call_argument_source` still keeps the legacy fallback when
  no explicit `FrameSlotValue` selection is present; this packet preserves that
  compatibility and proves it with focused dispatch coverage.
- Explicit `FrameSlotValue` selections now require `source_value_id`,
  `source_value_name`, `source_home_kind == StackSlot`, `source_slot_id`,
  `source_stack_offset_bytes`, `source_size_bytes`, and `source_align_bytes`.
- Do not retire callee-saved-register prior preservation in the same slice;
  `PreparedCallArgumentSourceSelection` only proves the stack-slot
  `PriorPreservation` source path for this consolidation step.
- Frame-slot address and local-frame address materialization complete
  selected-source paths are now consumed from prepared facts, and explicit
  incomplete selections fail closed. Absent-selection compatibility still uses
  legacy lookup.
- Stack-slot `PriorPreservation` explicit selections now require a complete
  stack payload; absent-selection prior-preservation lookup remains the
  compatibility path.
- SRET `FrameSlotAddress` explicit selections now require a complete
  frame-slot address payload; absent-selection SRET compatibility still uses
  `call_plan.memory_return`.
- Do not invent a new call-plan API under this source idea.
- Do not move AArch64 emission details into the shared planner.
- Do not claim progress through file concatenation, expectation weakening, or
  hidden helper rewrites.

## Proof

Ran delegated proof command:

`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } | tee test_after.log'`

Result: build succeeded and the backend subset passed, 162/162 tests.

Proof log: `test_after.log`.
