Status: Active
Source Idea Path: ideas/open/104_bir_prealloc_pointer_carrier_provenance_contract.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Pointer Carrier Seeds And Propagation

# Current Packet

## Just Finished

Step 1 inventory completed for
`src/backend/prealloc/regalloc/pointer_carriers.cpp` without implementation or
test edits.

Producer and propagation surfaces:

- Explicit pointer definitions are built from pointer parameters and pointer
  results of `BinaryInst`, `SelectInst`, `CastInst`, `PhiInst`,
  `LoadLocalInst`, `LoadGlobalInst`, and `CallInst`. This set suppresses later
  inferred plus/minus carrier derivation for already-explicit pointer values; it
  is a guard, not a provenance seed.
- `PreparedAddressBaseKind::PointerValue` in
  `PreparedAddressingFunction::accesses` seeds `direct_step_by_value_name` from
  each access `pointer_value_name` and `size_bytes`, then publishes self
  carriers `{base_value_name = value, byte_delta = 0, step_bytes = min size}`.
  This is the current structured prepared-memory-address authority route for
  `MemoryAddress::PointerValue`.
- BIR `Value::pointer_symbol_link_name_id` on named pointer results seeds a
  self carrier with `base_symbol_name` and `step_bytes = 1`. The current route
  copies target-neutral BIR symbol metadata into prealloc carrier state for
  later publication.
- Carrier lookup first prefers an existing `pointer_carriers[value]`, then a
  direct self carrier from `direct_step_by_value_name`, then a raw
  `slot_pointer_carriers[slot_name]` entry. The slot path is keyed only by
  local-slot spelling.
- Pointer `LoadLocalInst` without `MemoryAddress` can propagate a carrier from
  the result/direct map or from `slot_pointer_carriers[slot_name]` into the load
  result, and records the most recent loaded pointer by slot and by value.
- Pointer `LoadLocalInst` with `MemoryAddress`, and pointer `LoadGlobalInst`,
  only recover existing result/direct carrier state and update the recent-loaded
  pointer tracker. They do not use the local-slot carrier map.
- Pointer `StoreLocalInst` without `MemoryAddress` propagates an already-known
  stored pointer carrier into both `pointer_carriers[stored]` and
  `slot_pointer_carriers[slot_name]`.
- Pointer `StoreLocalInst` without `MemoryAddress` also derives a successor
  carrier for an unresolved, non-explicit stored value after a prior same-slot
  pointer load: base becomes the loaded pointer value name and `byte_delta`
  becomes `+step_bytes`. This is predecessor/successor inference from local
  instruction order plus raw slot spelling.
- Pointer `StoreLocalInst` with `MemoryAddress::PointerValue` derives a
  predecessor carrier for an unresolved, non-explicit stored value when a recent
  loaded pointer exists: base becomes that loaded pointer value name and
  `byte_delta` becomes `-step_bytes`.
- Pointer `StoreGlobalInst` with `MemoryAddress::PointerValue` derives the same
  predecessor `-step_bytes` carrier from the recent loaded pointer tracker.
- The iterative pass runs up to `function.blocks.size() * 4 + 1` rounds. The
  slot carrier map persists across rounds, while the recent-loaded maps reset
  per block each round. Candidate updates reject invalid/zero-step carriers,
  keep smaller self step sizes, and allow a non-self carrier to replace a self
  zero-delta carrier.

Consumer and authority classification:

- `classify_prepared_value_home` is the main consumer. For pointer values with a
  non-self or nonzero-delta carrier, it publishes
  `PreparedValueHomeKind::PointerBasePlusOffset` with
  `pointer_base_value_name`, optional `pointer_base_symbol_name`, and
  `pointer_byte_delta`, while preserving register/stack placement as physical
  prealloc metadata.
- Structured `PreparedAddressBaseKind::PointerValue` access facts are current
  target-neutral prepared-memory-address authority.
- BIR `pointer_symbol_link_name_id` is current target-neutral symbol provenance,
  but the conversion into carrier-map publication is implicit and should get a
  named contract in the next step.
- Local-slot load/store propagation without `MemoryAddress`, raw slot-name
  seeding, and predecessor/successor plus/minus inference are currently
  prealloc-local regalloc convenience. They become target-neutral-looking facts
  only when converted into `PointerBasePlusOffset` value homes, so this is the
  main authority gap to classify.
- Register assignment, stack-slot homes, decoded-home storage, publication-plan
  carrier movement, and target memory operand planning are physical/prepared
  consumers, not provenance authorities.

Existing proof surfaces and gaps:

- Prepared addressing tests cover `PreparedAddressBaseKind::PointerValue`
  publication for pointer-indirect local/global memory accesses.
- BIR notes tests cover preservation of `pointer_symbol_link_name_id` on
  pointer values through lowering.
- Prepared lookup, decoded-home, publication-plan, frame-stack-call, and AArch64
  MIR tests cover downstream consumption of `PointerBasePlusOffset`,
  `pointer_base_symbol_name`, and `pointer_byte_delta` when such homes already
  exist.
- Gap: there is no focused proof that every current `pointer_carriers.cpp`
  inference route is backed by structured authority before publishing
  `PointerBasePlusOffset`. The no-address local-slot and plus/minus inferred
  routes are the highest-risk surfaces.

## Suggested Next

Proceed to Step 2 and decide the narrow authority contract: which pointer
carrier facts must come from explicit BIR/prepared provenance, which
`pointer_carriers.cpp` state may remain transient regalloc convenience, and
which existing raw slot/order-derived routes must fail closed or be replaced.

## Watchouts

- Do not treat load/store instruction shape as target-neutral provenance
  authority without a structured fact.
- Keep register allocation, storage homes, spill decisions, and target
  placement in prealloc.
- Separate transient regalloc carrier metadata from durable BIR/prepared
  pointer provenance.
- Avoid adding slot-name parsing, testcase-shaped pointer pairing, or a contract
  that makes same-slot load/store order semantic authority.
- `base_symbol_name` currently only rides on carrier state; make sure Step 2
  distinguishes BIR symbol provenance from physical indirect-call or
  publication-plan movement.

## Proof

Passed:
`git diff --quiet -- src/backend/bir src/backend/prealloc src/backend/mir tests && printf 'analysis-only proof: no implementation or test diff for pointer carrier provenance inventory\n' > test_after.log`

Proof log: `test_after.log`

Result:
`analysis-only proof: no implementation or test diff for pointer carrier provenance inventory`
