Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Move Non-Emission Spelling To Its Owner

# Current Packet

## Just Finished

Lifecycle review found Step 2 exhausted after the latest committed
helper-boundary retirements. The completed Step 2 route retired the prepared
source-selection duplicate boundaries that had a valid mapping, including
explicit frame-slot value/address, local-frame address, stack-slot prior
preservation, SRET address, and complete byval register-lane source payload
cases.

The remaining audited source-helper paths are not valid Step 2 continuation
targets without new prepared facts or mapping proof: absent-selection
compatibility, fragmented byval fallback, callee-saved-register prior
preservation, and emission/spelling helpers. Step 2 should not be expanded to
cover those paths under the current runbook contract.

## Suggested Next

Advance execution to Step 3 of `plan.md`: identify call printing or effect
spelling still owned by the AArch64 calls surface, then delegate a narrow
behavior-preserving move to the owning machine-printer or shared MIR layer.

Do not continue Step 2 source-selection retirement unless a fresh mapping proves
another AArch64-local helper boundary duplicates prepared call-plan facts.

## Watchouts

- Step 3 is about non-emission spelling ownership, not another source-selection
  retirement pass.
- Do not retire absent-selection fragmented byval lane fallback logic outside
  `make_byval_register_lane_prepared_source`; the accepted prerequisite only
  covers explicit complete byval register-lane selections with prepared source
  payload.
- `selected_byval_lane_extent_bytes` now treats every explicit
  `source_selection` as authoritative for byval extent selection: complete
  `ByvalRegisterLane` provides the prepared extent; incomplete or wrong-kind
  explicit selections do not rederive a legacy extent. Absent-selection byval
  compatibility remains intact.
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
- SRET explicit selections now require a complete `FrameSlotAddress` payload;
  incomplete address selections and non-address selections fail closed instead
  of using `call_plan.memory_return`.
- Absent-selection SRET compatibility still uses `call_plan.memory_return`.
- `make_frame_slot_call_argument_address_source` now blocks all explicit
  non-`FrameSlotAddress` selections from falling back to legacy frame-slot
  address reconstruction. Absent-selection compatibility still uses the legacy
  address-materialization/stack-object lookup.
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
