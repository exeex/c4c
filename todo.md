Status: Active
Source Idea Path: ideas/open/01_shared_prepared_call_argument_source_selection.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Consume The Shared Selection In AArch64

# Current Packet

## Just Finished

Step 3 of `plan.md` made AArch64 `BeforeCall` argument lowering consume
`PreparedCallArgumentSourceSelection` when the shared fact is present and
complete, while keeping machine-node emission target-local.

Consumed source kinds:

- `FrameSlotValue`: `make_frame_slot_call_argument_source` now prefers the
  shared slot, offset, size, and alignment fact before consulting prepared
  addressing.
- `FrameSlotAddress`: frame-slot address lowering now prefers the shared
  address/materialization fact; the AArch64 dispatch test clears the old
  prepared-addressing input and still proves `add x1, sp, #48`.
- `LocalFrameAddressMaterialization`: local aggregate address lowering now
  prefers the shared materialization block, instruction, slot, offset, size,
  and alignment fact before scanning same-block materializations or local-slot
  objects.
- `PriorPreservation`: stack-slot prior preservation can now be sourced from
  the shared preserved stack-slot fields without reselecting the prior call.
- `ByvalRegisterLane`: byval lane extent and complete frame-slot payload source
  can now come from the shared selection before AArch64 reconstructs lane
  stores.

Deferred source kinds and reasons:

- `PriorPreservation` for callee-saved register preservation is deferred
  because `PreparedCallArgumentSourceSelection` does not carry the preserved
  register name, bank, occupied register set, or register placement needed for
  AArch64 register-source emission.
- `ByvalRegisterLane` fragmented lane reconstruction remains as a fallback when
  the shared fact has an extent but no complete source slot, stack offset, size,
  and alignment. The shared fact selects the source kind and extent; AArch64
  still owns target-local fragmented load/store emission.
- Legacy fallback scans remain for absent or incomplete shared facts so this
  prerequisite slice does not broaden into unsupported call-plan population
  repairs.

## Suggested Next

Execute Step 4 of `plan.md`: run the prerequisite acceptance checkpoint and
decide whether the parked AArch64 calls consolidation idea can be reactivated.

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
- Step 3 did not consolidate or remove AArch64 call files.
- The direct consumer assertion currently covers `FrameSlotAddress`; the other
  consumed kinds are covered through existing backend behavior plus the full
  backend subset. Step 4 should decide whether that is sufficient for
  acceptance or whether to request more per-kind consumer assertions.

## Proof

Delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed. `test_after.log` contains the canonical proof log with all 162
matching `backend_` tests passing.
