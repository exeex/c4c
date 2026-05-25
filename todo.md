Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Retire One Proven Duplicate Helper Boundary

# Current Packet

## Just Finished

Step 2 of `plan.md` retired one local-frame address materialization fallback
boundary. `make_local_frame_address_call_argument_source` now treats any
explicit `source_selection` as authoritative for that local-frame source
decision: a complete `LocalFrameAddressMaterialization` selection still emits
the selected prepared frame address, but incomplete or non-local selections no
longer fall through to AArch64-local prepared-addressing or stack-object
reconstruction.

Instruction-dispatch coverage now proves an explicit non-local
`FrameSlotValue` selection with matching legacy local-frame address facts
lowers through the ordinary prepared register move path instead of rederiving a
local frame address. Existing absent-selection coverage still preserves the
legacy local-frame address lookup path.

## Suggested Next

Continue Step 2 by auditing the next remaining selected-source fallback
boundary outside the now-closed local-frame address helper. Only retire it if
complete `PreparedCallArgumentSourceSelection` facts cover that source decision
and the no-`source_selection` compatibility path remains tested.

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
- `make_frame_slot_call_argument_source` still keeps the legacy fallback only
  when no explicit `source_selection` is present; this packet preserves that
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
  incomplete `FrameSlotAddress` selections through the shared frame-slot value
  helper. Absent-selection compatibility still permits the legacy frame-slot
  value load.
- `make_frame_slot_call_argument_source` now blocks all explicit
  non-`FrameSlotValue` selections from falling back to legacy frame-slot value
  reconstruction; this covers the audited FPR, binary128, and outgoing stack
  frame-slot value callers without adding per-call-site guards.
- Stack-slot `PriorPreservation` explicit selections now require a complete
  stack payload; absent-selection prior-preservation lookup remains the
  compatibility path.
- Explicit non-prior source selections no longer reuse an available prior
  stack-preserved home through the prior-preservation caller. This slice keeps
  unrelated incomplete frame-slot/local-frame diagnostics on their existing
  paths when no prior home is available.
- SRET `FrameSlotAddress` explicit selections now require a complete
  frame-slot address payload; absent-selection SRET compatibility still uses
  `call_plan.memory_return`.
- `make_local_frame_address_call_argument_source` now blocks all explicit
  non-`LocalFrameAddressMaterialization` selections from falling back to legacy
  local-frame address reconstruction. Absent-selection compatibility still
  uses the prepared-addressing/stack-object lookup.
- Do not invent a new call-plan API under this source idea.
- Do not move AArch64 emission details into the shared planner.
- Do not claim progress through file concatenation, expectation weakening, or
  hidden helper rewrites.

## Proof

Ran delegated proof command:

`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } | tee test_after.log'`

Result: build succeeded and the backend subset passed, 162/162 tests.

Proof log: `test_after.log`.
