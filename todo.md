Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Move Non-Emission Spelling To Its Owner

# Current Packet

## Just Finished

Step 3 moved prepared preserved-call effect conversion out of
`calls_printing.cpp` and into the machine-instruction record/effect layer.
`effect_from_prepared_call_preserved_value` is now a private helper in
`instruction.cpp` beside `effect_from_prepared_call_clobber`, and
`effects_from_prepared_call_preserved_values` is declared in `instruction.hpp`
beside `effects_from_prepared_call_clobbers`.

The call `InstructionRecord::preserves` population still uses the same
preserve-effect conversion under `publish_prepared_call_preserve_effects()`;
only the ownership location changed.

## Suggested Next

Have the supervisor review the Step 3 slice for route alignment and commit it,
then decide whether Step 3 has any remaining non-emission spelling owners or
whether lifecycle review should advance the plan.

## Watchouts

- Step 3 is about non-emission spelling ownership, not another source-selection
  retirement pass.
- Existing machine-printer and dispatch coverage exercises preserved register
  and stack-slot effects through `InstructionRecord::preserves`; this packet
  intentionally kept behavior unchanged rather than adding expectation churn.
- Do not move the immediate-cast inline-assembly text from `calls_moves.cpp` in
  this same slice; that path is emission-like lowering for selected call
  argument publication and has a less clear Step 3 owner.
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

Ran the delegated proof command:

`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } | tee test_after.log'`

Result: passed. The build completed and CTest reported 162/162 `^backend_`
tests passed.

Proof log: `test_after.log`.
