Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Retire One Proven Duplicate Helper Boundary

# Current Packet

## Just Finished

Step 2 of `plan.md` retired one explicit `FrameSlotAddress` fallback boundary
in the non-byval GPR frame-slot call-argument path. After
`make_sret_memory_return_address_source` and
`make_frame_slot_call_argument_address_source` reject an explicit incomplete
`PreparedCallArgumentSourceSelectionKind::FrameSlotAddress`, the GPR
frame-slot register publication now fails closed instead of falling through to
`make_frame_slot_call_argument_source` and rederiving a value load from the
legacy frame-slot home.

The instruction-dispatch coverage now includes an incomplete explicit
`FrameSlotAddress` selection for a frame-slot aggregate-address argument into
`x1` while matching legacy value-home facts remain available;
`lower_before_call_moves` fails with `MissingValueAuthority` instead of loading
the frame-slot value. A paired absent-selection case proves the retained
no-`source_selection` compatibility path still uses the legacy frame-slot value
source.

## Suggested Next

Continue Step 2 by auditing the remaining non-byval frame-slot call sites that
still call `make_frame_slot_call_argument_source`, especially FPR, binary128,
and stack-slot publication paths, for exactly one explicit selected-source
boundary whose complete prepared facts fully cover the source decision. Keep the
next slice paired with explicit incomplete-selection failure coverage and
absent-selection compatibility coverage.

## Watchouts

- Do not retire absent-selection fragmented byval lane fallback logic outside
  `make_byval_register_lane_prepared_source`; the accepted prerequisite only
  covers explicit complete byval register-lane selections with prepared source
  payload.
- Register-homed byval lane explicit selections now fail closed when the
  prepared-source helper lacks a complete payload. The absent-selection
  register-home aggregate address path remains as compatibility fallback.
- Stack-slot byval lane explicit selections into outgoing stack arguments now
  fail closed when the prepared-source helper lacks a complete payload. The
  absent-selection stack-lane frame-slot path remains as compatibility
  fallback.
- Stack-slot byval lane explicit selections into call ABI registers now fail
  closed when the prepared-source helper lacks a complete payload. The
  absent-selection stack-home frame-slot load path remains as compatibility
  fallback.
- Indirect large-byval explicit selections now fail closed when the
  prepared-source helper lacks a complete payload. The absent-selection
  indirect-byval frame-slot path remains as compatibility fallback.
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
- The non-byval GPR frame-slot register path now fails closed for explicit
  incomplete `FrameSlotAddress` selections before value-source fallback.
  Absent-selection compatibility still permits the legacy frame-slot value load.
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
