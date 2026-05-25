Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Reactivation Prerequisite Mapping

# Current Packet

## Just Finished

Step 1 of `plan.md` completed as a mapping-only audit of remaining AArch64
call move/source/preservation helpers against available shared prepared
call-plan facts.

Helper-to-prepared-fact mapping:

- `lower_before_call_move`: already reads
  `PreparedCallBoundaryMoveClassification` from
  `classify_prepared_call_boundary_move`, plus
  `PreparedCallArgumentPlan` and `PreparedAbiBinding` for selected
  `BeforeCall` argument moves. This proves destination ABI binding and broad
  argument-plan identity are shared facts.
- `make_frame_slot_call_argument_source`: partially maps to
  `PreparedCallArgumentPlan::source_encoding == FrameSlot`,
  `source_slot_id`, `source_stack_offset_bytes`, and the current
  `PreparedValueHome`. It still constructs the target memory operand locally.
- `make_sret_memory_return_address_source`: maps to
  `PreparedCallPlan::memory_return` facts (`sret_arg_index`, `encoding`,
  `slot_id`, `stack_offset_bytes`, size, and alignment).
- `make_frame_slot_call_argument_address_source`: can consume
  `PreparedAddressMaterialization` when one exists, but falls back to local
  stack-object scanning under `allows_local_aggregate_address_publication`.
  That fallback is not an already-prepared call-argument source choice.
- `make_local_frame_address_call_argument_source`: can consume
  `PreparedAddressMaterialization` when one exists, but also falls back to
  local stack-object/name matching. That fallback is not an already-prepared
  call-argument source choice.
- `make_byval_register_lane_prepared_source` and
  `collect_byval_register_lane_stores`: depend on local BIR scan plus prepared
  memory-access/addressing facts to reconstruct byval register-lane source
  bytes. No shared call-plan fact currently selects this source for the
  `BeforeCall` move.
- `make_prior_preserved_call_argument_source` with
  `find_prior_preserved_value_for_call_argument`: maps to
  `PreparedCallPreservedValue` and `PreparedCallPlanLookups`
  (`find_latest_indexed_prior_preserved_value`), but the final choice that a
  selected call-argument move should source from that prior preservation is
  still made locally in `lower_before_call_move`.
- `make_callee_saved_preservation_home_population` and
  `make_callee_saved_preservation_home_republication_instruction`: map to
  `PreparedCallBoundaryEffectPlan` entries from
  `plan_prepared_call_boundary_effects` with
  `PreservationHomePopulation` and `PreservationRepublication`. These are
  already shared prepared facts for preservation effects, not for the remaining
  selected call-argument source problem.

Conclusion: the reactivation prerequisite is not satisfied. Existing shared
facts classify call-boundary moves and expose argument, destination,
preservation, memory-return, address-materialization, and byval-adjacent facts,
but there is still no single prepared call-argument source fact that says a
selected `BeforeCall` argument move should source from prior preservation,
local-frame address materialization, frame-slot/address sources, or byval
register-lane materialization. No AArch64-local cleanup route is actionable
under this source idea from the current mapping.

## Suggested Next

Block this runbook or route back to shared call-plan authority work to add the
missing prepared call-argument source-selection fact before attempting Step 2.

## Watchouts

- Do not perform another AArch64-local move/source cleanup checkpoint unless
  the prerequisite prepared fact exists or a fresh mapping proves a different
  already-prepared duplicate.
- Do not invent a new call-plan API under this source idea.
- Do not claim progress through file concatenation, expectation weakening, or
  hidden helper rewrites.
- The already-available duplicate boundary is preservation effects via
  `PreparedCallBoundaryEffectPlan`; the remaining blocked boundary is selected
  `BeforeCall` argument source choice.

## Proof

Mapping-only packet. Used `c4c-clang-tools` symbol/signature queries and
targeted source reads; no implementation files were modified, so no build,
ctest, or `test_after.log` was required by the delegated proof.
