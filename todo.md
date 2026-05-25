Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Reactivation Mapping With Prepared Source Selection

# Current Packet

## Just Finished

Step 1 of `plan.md` mapped the remaining AArch64 call helper boundaries
against the prepared source-selection facts now available on
`PreparedCallArgumentPlan::source_selection`.

Exact helper-to-prepared-fact mapping:

- `make_frame_slot_call_argument_source` in
  `src/backend/mir/aarch64/codegen/calls_argument_sources.cpp` duplicates the
  `FrameSlotValue` path. Prepared facts already carry
  `source_slot_id`, `source_stack_offset_bytes`, `source_size_bytes`, and
  `source_align_bytes`; the remaining local fallback still re-finds a unique
  prepared memory access by result value or falls back to the argument/home
  stack offset.
- `make_sret_memory_return_address_source` and
  `make_frame_slot_call_argument_address_source` duplicate the
  `FrameSlotAddress` path. Prepared facts already carry either the memory-return
  slot/offset/size/alignment or the selected frame-slot address
  materialization fields; the remaining local fallback still scans prepared
  address materializations and reconstructs local aggregate address
  publication.
- `make_local_frame_address_call_argument_source` duplicates the
  `LocalFrameAddressMaterialization` path. Prepared facts already carry the
  selected address-materialization block label, instruction index, frame-slot
  id, byte offset, size, and alignment; the remaining local fallback still
  matches materialized value names and searches local-frame stack objects.
- `make_prior_preserved_call_argument_source` plus the
  `lower_call_boundary_move` preservation branch duplicates the stack-slot
  `PriorPreservation` path. Prepared facts already carry the selected value id,
  value name, preservation route, preserved stack slot, offset, size, and
  alignment; the local fallback still asks call-plan lookups to choose a prior
  preserved value. This route is only proven for stack-slot preservation; the
  selected-source fact intentionally does not yet retire callee-saved-register
  preservation.
- `make_byval_register_lane_prepared_source` duplicates the complete
  `ByvalRegisterLane` path. Prepared facts already carry
  `byval_lane_extent_bytes`, source slot, stack offset, source size, and
  alignment; the remaining fallback still recollects lane stores from local
  addressing facts. Fragmented byval fallback paths remain outside the proven
  mapping.

First valid Step 2 implementation slice:

Retire the fallback reconstruction inside
`make_byval_register_lane_prepared_source` for complete `ByvalRegisterLane`
selections, leaving it to accept only the prepared selection path and to fail
closed when the prepared fact is absent or incomplete. This is the narrowest
coherent slice because the helper already has a complete prepared-selection
fast path, the source-selection fact carries the required lane extent and
frame-slot payload, and the slice does not require changing the shared
call-plan API.

## Suggested Next

Execute Step 2 of `plan.md` for the byval register-lane helper boundary:
remove the local `collect_byval_register_lane_stores` fallback from
`make_byval_register_lane_prepared_source`, keep the prepared
`ByvalRegisterLane` path behavior-preserving, and run a fresh build plus the
focused backend call tests selected by the supervisor.

## Watchouts

- Do not retire fragmented byval lane fallback logic outside
  `make_byval_register_lane_prepared_source`; the accepted prerequisite only
  covers complete byval register-lane selections with prepared source payload.
- Do not retire callee-saved-register prior preservation in the same slice;
  `PreparedCallArgumentSourceSelection` only proves the stack-slot
  `PriorPreservation` source path for this consolidation step.
- Frame-slot value/address and local-frame address materialization fallbacks
  are also mapped, but they have broader helper interactions and should follow
  after the byval slice proves the pattern.
- Do not invent a new call-plan API under this source idea.
- Do not move AArch64 emission details into the shared planner.
- Do not claim progress through file concatenation, expectation weakening, or
  hidden helper rewrites.

## Proof

Mapping-only packet; no build was run and no `test_after.log` was produced.
Proof is `git diff -- todo.md` plus code inspection with:

- `c4c-clang-tool-ccdb list-symbols` on
  `calls_argument_sources.cpp`, `calls_moves.cpp`,
  `calls_byval_aggregates.cpp`, and `call_plans.cpp`
- `rg -n "PreparedCallArgumentSourceSelection|source_selection|Byval|PriorPreserved|FrameSlot|LocalFrame|ArgumentSource" ...`
- targeted `sed -n` reads around `PreparedCallArgumentSourceSelection`,
  `select_prepared_call_argument_source`,
  `make_selected_frame_slot_source`,
  `make_byval_register_lane_prepared_source`, and prior-preservation helpers
