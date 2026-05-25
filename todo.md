Status: Active
Source Idea Path: ideas/open/01_shared_prepared_call_argument_source_selection.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Add Shared Prepared Source-Selection Fact

# Current Packet

## Just Finished

Step 2 of `plan.md` added the target-independent
`PreparedCallArgumentSourceSelection` fact to `PreparedCallArgumentPlan` and
populates it during shared call planning without changing AArch64 emission.

Implemented fields:

- `kind` covers `None`, `PriorPreservation`,
  `LocalFrameAddressMaterialization`, `FrameSlotAddress`, `FrameSlotValue`,
  and `ByvalRegisterLane`.
- Shared source identity and home fields: `source_value_id`,
  `source_value_name`, `source_home_kind`, `source_slot_id`,
  `source_stack_offset_bytes`, `source_size_bytes`, `source_align_bytes`,
  `source_base_value_id`, and `source_pointer_byte_delta`.
- Local frame/address materialization fields:
  `address_materialization_block_label`,
  `address_materialization_inst_index`,
  `address_materialization_frame_slot_id`, and
  `address_materialization_byte_offset`.
- Prior-preservation fields: `preserved_call_block_index`,
  `preserved_call_instruction_index`, `preservation_route`, and preserved stack
  slot fields.
- Byval lane fields: `byval_lane_extent_bytes` and
  `byval_lane_source_instruction_index`.

Covered source kinds:

- `FrameSlotValue` is populated from prepared value homes and unique prepared
  memory accesses; the prepared-plan contract test now asserts a byval
  frame-slot argument publishes this fact.
- `FrameSlotAddress` is populated for matching frame-slot address
  materializations and for sret memory-return slots when the selected call
  argument source is frame-slot encoded in the shared plan.
- `LocalFrameAddressMaterialization` is populated from matching
  `PreparedAddressMaterializationKind::FrameSlot` facts in the same prepared
  block at or before the call.
- `PriorPreservation` is populated from the latest prior
  `PreparedCallPreservedValue` in the current prepared call-plan function.
- `ByvalRegisterLane` is populated for existing
  `call_arg_byval_aggregate_register_lanes` move reasons with prepared byval
  ABI size and selected source-home extent capped at 16 bytes.

Deferred cases:

- The Step 2 tests visibly assert `FrameSlotValue`; the other kinds are
  represented and populated but not all have dedicated per-kind assertions yet.
  Step 3 should add consumer-side assertions while replacing AArch64 local
  source-choice reads.
- Byval lane store-fragment coverage still uses the selected source-home extent
  and ABI lane count in shared planning; detailed lane-store reconstruction
  remains available to AArch64 until Step 3 consumption proves the exact shared
  fact requirements.

## Suggested Next

Execute Step 3 of `plan.md`: consume
`PreparedCallArgumentSourceSelection` from AArch64 `BeforeCall` argument
lowering, replacing local source-choice reconstruction only for cases where the
shared fact is present and complete.

## Watchouts

- Do not consolidate or delete AArch64 `calls*.cpp` files in this prerequisite
  runbook.
- Do not encode AArch64-specific machine-node emission details in the shared
  prepared fact.
- Do not claim progress through testcase-shaped matching, expectation
  weakening, or wrapper-only renames of the existing local decision tree.
- The first implementation slice should not move destination-register or
  destination-stack selection into the new fact. The new fact owns source
  choice only.
- `ByvalRegisterLane` is allowed to carry the existing target-neutral move
  reason as source classification input, but it must publish the resulting
  selected source and extent as shared prepared data so AArch64 no longer owns
  that source-choice decision after Step 3.
- `FrameSlotAddress`, `LocalFrameAddressMaterialization`,
  `PriorPreservation`, and `ByvalRegisterLane` should get direct consumer
  assertions during Step 3; Step 2 kept AArch64 behavior unchanged.

## Proof

Delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed. `test_after.log` contains the canonical proof log with all 162
matching `backend_` tests passing.
