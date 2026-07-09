Status: Active
Source Idea Path: ideas/open/650_edge_store_local_aggregate_publication_ordering.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Locate The Publication-Ordering Boundary

# Current Packet

## Just Finished

Completed Step 2 boundary location for the `%t38.phi` / `%t17.phi`
edge-store carrier failures in `pr68185.c` and `pr68321.c`.

Classification: the narrow missing owner is RV64 consumer admission for
authoritative `edge_store_slot` join-transfer carriers, not producer
destination-access publication and not a split owner at this point.

The producer side already has the facts needed to distinguish the route:

- `pr68185.c`: `%t38` is register-homed in `t0`; the join transfer for
  `logic.end.34` has `carrier=edge_store_slot`,
  `ownership=authoritative_branch_pair`, storage `%t38.phi`, and two
  published predecessor move bundles for `%t36 -> %t38` and `0 -> %t38`.
- `pr68321.c`: `%t17` is register-homed in `t0`; the join transfer for
  `logic.end.13` has `carrier=edge_store_slot`,
  `ownership=authoritative_branch_pair`, storage `%t17.phi`, and two
  published predecessor move bundles for `%t15 -> %t17` and `1 -> %t17`.
- `find_store_source_publication_access(...)` cannot find a
  `PreparedMemoryAccess` for `%t38.phi` / `%t17.phi`, so
  `plan_prepared_store_source_publication(...)` reports
  `MissingDestinationAccess`. That status is a symptom of sending the virtual
  phi carrier through local-memory publication, not proof that a real frame
  access is missing.
- The prepared stack/value-home evidence has ordinary scalar frame slots for
  `%lv.*` locals and register homes for the phi results; there is no real stack
  object for `%t38.phi` or `%t17.phi` in the focused object route.

RV64 currently fails because `fragment_for_prepared_instruction(...)` handles
the carrier `StoreLocalInst` and later carrier `LoadLocalInst` through
`prepared_memory_access_for_local_instruction(...)` and then
`fragment_for_prepared_store_local(...)` /
`fragment_for_prepared_load_local(...)`. With no prepared memory access for the
virtual carrier, diagnostics report `unsupported_local_memory_access` with
`access_base=none`.

Owned implementation surface for Step 3 should stay in
`src/backend/mir/riscv/codegen/object_emission.cpp`, around:

- the `StoreLocalInst` and `LoadLocalInst` branches in
  `fragment_for_prepared_instruction(...)`;
- existing edge-publication helpers such as
  `prepared_join_transfer_edge_copies_are_published(...)`,
  `consume_edge_publication_move_intent(...)`, and the
  `prepared_predecessor_select_publication_bundle_*` admission/materialization
  helpers;
- a narrow helper if needed to identify a `StoreLocalInst` / `LoadLocalInst`
  whose local slot name exactly matches an authoritative
  `PreparedJoinTransfer` with `carrier_kind=EdgeStoreSlot`, published
  predecessor edge copies, and a register-homed destination value.

Do not repair this by synthesizing a fake `PreparedMemoryAccess` for
`%t38.phi` or `%t17.phi`; that would misclassify an out-of-SSA edge carrier as
ordinary scalar frame-slot local memory.

Focused positive test target shape: a backend RV64 object case with an `i32`
phi lowered to an `edge_store_slot` carrier where one predecessor stores a
register-produced value and the other stores an immediate into the carrier
slot, the phi result is GPR-homed, and the object route emits/materializes the
predecessor edge moves into the destination register while suppressing the
carrier `store_local %*.phi` and `load_local %*.phi`. The test should reject
the current `unsupported_local_memory_access` path and should not require a
real frame-slot access for the carrier.

## Suggested Next

Proceed to Step 3 by implementing the narrow RV64 consumer admission for
authoritative `edge_store_slot` carriers in `object_emission.cpp`, or stop and
split only if implementation proves the current prepared control-flow facts do
not identify the carrier store/load and destination register precisely enough.

## Watchouts

- Preserve fail-closed behavior for ambiguous predecessor order, missing
  source-branch/source-transfer indexes, or non-authoritative join-transfer
  ownership.
- Reject missing destination ownership or missing/non-register destination
  homes unless a separately owned stack-destination route is explicitly in
  scope.
- Require the carrier `StoreLocalInst` / `LoadLocalInst` slot name to match
  the join-transfer storage name; lane, slot, or result mismatches must remain
  unsupported.
- Require published predecessor edge-copy facts and fresh source intent; stale
  source values or unavailable edge-publication move intents must remain
  fail-closed.
- Scalar-only frame-slot facts for `%lv.*` locals must not authorize
  edge-store carrier elision, and edge-store carrier authority must not broaden
  ordinary `StoreLocalInst` / `LoadLocalInst` support.
- Keep direct global-symbol local memory, aggregate global-object
  materialization, string-label local memory, and stack-home aggregate policy
  out of this packet unless a later diagnostic shows a distinct downstream
  owner.

## Proof

No build or CTest proof was required or run for this diagnostic-only packet,
and `test_after.log` was not overwritten.

Boundary evidence came from:

```sh
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/prealloc/publication_plans.cpp build/compile_commands.json
c4c-clang-tool-ccdb find-definition /workspaces/c4c/src/backend/prealloc/publication_plans.cpp find_store_source_publication_access build/compile_commands.json
c4c-clang-tool-ccdb find-definition /workspaces/c4c/src/backend/prealloc/publication_plans.cpp plan_prepared_store_source_publication build/compile_commands.json
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/riscv/codegen/object_emission.cpp build/compile_commands.json
c4c-clang-tool-ccdb find-definition /workspaces/c4c/src/backend/mir/riscv/codegen/object_emission.cpp prepared_memory_access_for_local_instruction build/compile_commands.json
```

The Step 1 artifacts under
`build/agent_state/650_step1_edge_store_evidence/` remain the representative
diagnostic packet for this boundary.
