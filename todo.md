Status: Active
Source Idea Path: ideas/open/463_select_edge_ule_source_producer_rematerialization.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define ULE Source-Producer Rematerialization Contract

# Current Packet

## Just Finished

Completed Step 2 contract definition for idea 463. A sound RV64
source-producer rematerialization contract exists only if prepared metadata
explicitly proves the duplicate carrier selects are carrier-only aliases of the
same join-transfer result. That fact is missing today, so no RV64 consumer
packet is justified yet.

Contract/blocker table:

| Field | Value |
| --- | --- |
| Accepted edge/source route | `pre_terminator_copies` edge `logic.rhs.end.40 -> logic.end.41`, move `%t46 -> %t50`, prepared edge publication with `source_producer_kind=binary`, source `%t46 = bir.ule ptr %t42, %t45`, destination `%t50` |
| Required carrier proof | Producer-owned prepared metadata must prove `%t50.phi.sel0` and `%t50.phi.sel1` are carrier-only aliases for the same join-transfer result `%t50` |
| Required operand proof | `%t42` and `%t45` must each be target-consumable at the predecessor-edge site through a GPR home, immediate/null, or explicit dependency-operand authority |
| Required RV64 behavior | Rematerialize the unsigned pointer `ule` compare at the predecessor-edge move site into the `%t50` destination register; never copy successor-defined `%t46` |
| Rejected adjacent shapes | Raw-name carrier inference, value-id-only matching, plain `%t46 -> %t50` copy/no-op, unapproved non-carrier uses, stale stack-load authority, generic move-bundle support, non-edge or mismatched publication |
| Exact blocker | No durable prepared carrier-alias record currently proves the duplicate `%t50.phi.sel0` / `%t50.phi.sel1` values are aliases of final `%t50`; current RV64 code therefore correctly rejects them in `prepared_select_edge_binary_source_has_only_carrier_uses` |
| Step 3 disposition | Route or split to a producer/prepared metadata packet for explicit duplicate-carrier alias authority before any RV64 rematerialization consumer is widened |

Artifact:
`build/agent_state/463_step2_select_edge_ule_source_producer_contract/contract.md`.

## Suggested Next

Execute Step 3 from `plan.md` as a routing/blocker packet, not an RV64 lowering
packet: select or split the producer/prepared metadata work needed to publish
explicit duplicate-carrier alias authority for `%t50.phi.sel0` /
`%t50.phi.sel1`.

Suggested artifact directory:
`build/agent_state/463_step3_select_edge_ule_source_producer_route/`.

## Watchouts

- Do not edit RV64 implementation files until explicit carrier-alias metadata
  exists.
- Do not implement a plain `%t46 -> %t50` copy or same-register no-op.
- Do not infer predecessor availability from prepared value homes alone.
- Do not infer duplicate-carrier authority from `%t50.phi.sel0` /
  `%t50.phi.sel1` names, raw select shape, value ids, block labels, or dump
  order.
- Do not add generic stack-to-register, register-to-register, or all-purpose
  move-bundle support.
- Do not consume `load_from_stack_slot missing_stack_freshness`.
- Do not reopen ideas 456, 458, 459, 460, 461, or 462 without new
  coordinate-bearing evidence that their exact route owns the first failure.
- Do not match on testcase name, function name, block label, value ids alone,
  raw BIR shape, or prepared dump order.
- Do not modify `test_baseline.new.log`, `test_baseline.log`,
  `test_before.log`, `test_after.log`, or `review/`.

## Proof

Step 2 proof:

```sh
git diff --check
```

Result: passed.
