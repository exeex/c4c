Status: Active
Source Idea Path: ideas/open/110_call_planning_frame_address_materialization_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Close The Computed Pointer-Carrier Caveat

# Current Packet

## Just Finished

Completed the read-only Step 2 computed pointer-carrier caveat audit in
`plan.md`.

Disposition:

- Existing materialization covers the supported local-frame computed-address
  route. No named producer gap was found for supported non-binary
  pointer-carrier bases reaching call planning as `PointerBasePlusOffset`
  without explicit `PreparedAddressMaterializationKind::FrameSlot` authority.
- `select_prepared_call_argument_source()` consumes computed local aggregate
  arguments only when the call argument allows local aggregate address
  publication, the source encoding is `ComputedAddress`, and the source home is
  `PreparedValueHomeKind::PointerBasePlusOffset` with a
  `pointer_base_value_name`. It first calls
  `find_latest_frame_slot_materialization(..., selected_source_value_name,
  true)` and only then falls back to `find_local_frame_address_source()`.
- `build_prepared_value_homes()` calls `build_pointer_carrier_map()` and assigns
  `PointerBasePlusOffset` only for pointer values with semantic carrier
  authority whose base differs from the value or whose delta is nonzero. Local
  pointer load results that are memory bases or risky cycle loads are excluded
  from this home shape.

Supported computed-address producers inspected:

- `PreparedFrameAddressMaterialization`: seeded from existing frame-slot address
  materializations in `build_pointer_carrier_map()`. Immediate pointer
  arithmetic over that base becomes `BirPointerImmediateOffset`, and the call
  consumer's selected base is the materialized frame-address value, so explicit
  frame-slot materialization is available before the computed argument is
  selected.
- `BirPointerImmediateOffset`: the only producer that creates an offset carrier
  from BIR pointer arithmetic. It supports pointer add/sub with an immediate
  integer delta and requires the base to resolve to an existing semantic
  carrier; it does not mint authority from an arbitrary local name.
- `PreparedPointerValueAccess`: seeded from prepared pointer-value memory
  accesses. It can be transported through acyclic local-slot copies and can feed
  immediate-offset carriers, but it is not local-frame materialization
  authority by itself. If its base is not a frame-address materialization, the
  computed local-frame consumer has no prepared frame-slot fact to select; for
  non-local/symbol/access bases the later name fallback cannot match a local
  stack object.
- `BirPointerSymbol`: seeded only from structured pointer symbol metadata and
  transported through acyclic local-slot copies or immediate offsets. It is
  non-local symbol authority; it does not create a local frame-address fallback
  match.
- Local-slot load/store propagation: supports only transport of an existing
  semantic carrier through unaddressed, non-risky pointer slots. It does not
  introduce a new non-binary computed local-frame base; addressed predecessor
  inference and ambiguous/risky slots fail closed.

Producer/materialization evidence:

- `apply_frame_address_publication_hints()` marks named pointer values from
  binary/select/cast/phi/call/load/store shapes as frame-address publication
  candidates, so local aggregate pointer names that participate in these shapes
  can become stack-object `frame_address_value_name` facts.
- `append_frame_slot_address_materialization()` emits
  `PreparedAddressMaterializationKind::FrameSlot` when a named pointer result
  matches a frame-slot publication fact.
- `append_address_materializations()` reaches the frame-slot producer for
  binary pointer operands, call arguments, and store-local values. For computed
  local aggregate arguments, the supported offset shape is immediate pointer
  arithmetic over an already authoritative base; the binary path materializes
  the frame-address base used by the computed call argument.
- `find_latest_frame_slot_materialization()` ignores future facts and returns
  null on same-instruction ambiguity, so missing or conflicting explicit
  authority cannot be silently selected as prepared materialization.

Coverage/evidence checked:

- `check_pointer_carrier_contract_value_homes()` verifies symbol carriers,
  prepared pointer-value carriers, local-slot carrier transport, fail-closed
  missing symbol metadata, fail-closed addressed predecessor inference, and the
  frame-address offset case where prepared frame-address materialization
  authority seeds `PointerBasePlusOffset`.
- `check_rooted_pointer_binary_local_slot_activation()` verifies rooted pointer
  binary publication creates a `FrameSlot` materialization with frame slot id
  and byte offset.
- `check_local_frame_address_source_selection_contract()` and
  `check_derived_local_frame_address_source_selection_contract()` still accept
  either register or computed homes, so Step 4 should add narrower source-shape
  contracts before removing or bounding the remaining name fallback.

## Suggested Next

Proceed to Step 3 by replacing or narrowing the name-derived call-planning
fallback. The next coherent packet should update only
`select_prepared_call_argument_source()` so supported local aggregate call
arguments require explicit frame-slot materialization authority, while any
retained fallback is bounded to compatibility/unsupported behavior.

## Watchouts

- The direct `FrameSlotAddress`, register-source, and computed-address branches
  still contain `find_local_frame_address_source()` fallback code today.
- Computed non-local carriers such as symbols or pointer-value memory access
  bases are valid `PointerBasePlusOffset` homes, but they are not local frame
  addresses; do not treat their lack of frame-slot materialization as a local
  aggregate producer gap.
- Existing call-planning contracts do not cleanly separate register-derived and
  computed-address local-frame source selection, so Step 4 should lock those
  shapes down after Step 3 changes the consumer.

## Proof

Delegated proof command:
`bash -lc 'git diff --check > test_after.log 2>&1'`.

Result: passed; `test_after.log` contains the exact command output.
