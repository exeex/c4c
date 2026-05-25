Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Retire One Proven Duplicate Helper Boundary

# Current Packet

## Just Finished

Step 2 of `plan.md` retired the explicit
`PreparedCallArgumentSourceSelectionKind::ByvalRegisterLane` selected-source
fallback boundary for register-homed AArch64 byval register-lane call
arguments. When an explicit byval lane selection is present, the register-home
path now requires the complete prepared payload accepted by
`make_byval_register_lane_prepared_source`; it no longer rebuilds the payload
source from the legacy aggregate address register after the prepared-source
helper rejects an incomplete selection.

The instruction-dispatch coverage now includes an incomplete explicit
`ByvalRegisterLane` selection while matching legacy register-home facts are
still present; `lower_before_call_moves` fails closed instead of rederiving the
source from the register home. A paired absent-selection case proves the
retained no-`source_selection` compatibility path still lowers through the
legacy register-home aggregate address.

## Suggested Next

Continue Step 2 by auditing the remaining selected-source helper surfaces for a
small complete-payload-only boundary that can be retired without touching
absent-selection compatibility. A good next packet is to look at the remaining
stack-slot byval lane and indirect-byval-adjacent fallbacks, but only where
prepared source facts fully cover the path.

## Watchouts

- Do not retire fragmented byval lane fallback logic outside
  `make_byval_register_lane_prepared_source`; the accepted prerequisite only
  covers complete byval register-lane selections with prepared source payload.
- Register-homed byval lane explicit selections now fail closed when the
  prepared-source helper lacks a complete payload. The absent-selection
  register-home aggregate address path remains as compatibility fallback.
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
