Status: Active
Source Idea Path: ideas/open/465_rv64_ule_select_edge_rematerialization_consumer.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Carrier-Alias Consumer Surface

# Current Packet

## Just Finished

Completed Step 1 audit for idea 465. The producer/prepared carrier-alias
authority is available in `publication_plans`, but the RV64 object route does
not yet collect or pass `PreparedSelectCarrierAliasAuthorityRecords` into the
select-edge source-producer consumer. The representative operands `%t42` and
`%t45` have GPR homes and are not the first blocker.

Audit table:

| Surface | Evidence | Classification |
| --- | --- | --- |
| Carrier-alias records | Idea 464 added `PreparedSelectCarrierAliasAuthorityRecords` plus `collect_prepared_select_carrier_alias_authorities(prepared)`. | Producer metadata exists and is the required authority source. |
| RV64 collection point | `object_emission.cpp` currently collects dependency operand authorities and select-edge source-producer placements near object-function setup. | Missing consumer wiring: carrier-alias records are not collected or passed to move-bundle/source-producer materialization. |
| Current selected-source consumer | `fragment_for_prepared_select_edge_source_producer` consumes edge publication intent and dependency operand authorities. | It has no carrier-alias authority input. |
| Current alias/use guard | `prepared_select_edge_binary_source_has_only_carrier_uses(...)` requires select users to classify as final `PreparedJoinTransferCarrier` and match the join result. | This remains too narrow for duplicate carrier aliases such as `%t50.phi.sel0` / `%t50.phi.sel1`; Step 2 should replace/augment it with record-based authority. |
| Coordinate-bearing event | `pre_terminator_copies`, `logic.rhs.end.40 -> logic.end.41`, move `%t46 -> %t50`, value ids `20 -> 21`, register destination. | Matches the target publication route; no new diagnostic blocker. |
| `%t42` operand | Prepared home `value_id=18 kind=register reg=s1`; storage `encoding=register bank=gpr reg=s1`. | Target-consumable by existing register operand path. |
| `%t45` operand | Prepared home `value_id=19 kind=register reg=s2`; storage `encoding=register bank=gpr reg=s2`. | Target-consumable by existing register operand path. |
| Current rejection point | Existing route can match the edge publication, but rejects before rematerialization because duplicate carrier uses are not accepted by the old final-carrier-only guard. | Step 2 can define a bounded RV64 consumer contract; no producer/prepared metadata split is needed first. |

Artifact:
`build/agent_state/465_step1_carrier_alias_consumer_audit/audit.md`.

## Suggested Next

Execute Step 2 from `plan.md`: define the RV64 ULE rematerialization consumer
contract. The first bounded packet should wire explicit
`PreparedSelectCarrierAliasAuthorityRecords` into the select-edge
source-producer route, match records by function/edge/source/destination/source
producer, and require `%t42` / `%t45` to remain target-consumable before
emitting ULE rematerialization.

## Watchouts

- Do not implement during Step 2; keep it contract-only unless explicitly
  delegated otherwise.
- Do not infer duplicate-carrier aliases from `%*.phi.sel*` spelling, raw
  select shape, value ids, block labels, function names, testcase names, or
  dump order.
- Do not implement a plain `%t46 -> %t50` copy or same-register no-op.
- Do not add generic stack-to-register, register-to-register, or all-purpose
  move-bundle support.
- Do not consume `load_from_stack_slot missing_stack_freshness`.
- Do not modify `test_baseline.new.log`, `test_baseline.log`,
  `test_before.log`, `test_after.log`, or `review/`.

## Proof

Step 1 proof:

```sh
git diff --check
```

Result: passed.
