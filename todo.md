Status: Active
Source Idea Path: ideas/open/464_select_carrier_alias_metadata.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Duplicate Carrier Alias Metadata Gap

# Current Packet

## Just Finished

Completed Step 1 audit for idea 464. Existing prepared state has the join
transfer, edge transfer, source-producer lookup, edge publication, and
placement records needed around the duplicate carrier shape, but no durable
carrier-alias metadata surface proves `%t50.phi.sel0` / `%t50.phi.sel1` are
aliases of final `%t50`.

Audit table:

| Field | Value |
| --- | --- |
| Existing join facts | `PreparedJoinTransfer` carries function, join block, final result, select-materialization carrier kind, edge transfers, and source transfer indexes |
| Existing edge/source facts | `PreparedEdgePublication` carries predecessor/successor, source/destination values and ids, source producer kind/site, move, join transfer, edge transfer, and carrier kind |
| Existing producer facts | `select_chain_lookups` publishes per-value source producers, including `SelectMaterialization` for select results and `Binary` for `%t46` |
| Existing placement facts | `PreparedSelectEdgeSourceProducerPlacement` records source-producer suppression placement for before-instruction bundles, not duplicate carrier aliases |
| Current classifier limit | `prepared_join_transfer_matches_select` requires `select.result.name == join_transfer.result.name`, so `%t50.phi.sel0` / `%t50.phi.sel1` classify as ordinary selects rather than aliases |
| First missing surface | Producer-owned prepared carrier-alias record keyed by function, edge, join transfer, source value, destination value, carrier values, and source producer |
| Step 2 readiness | A carrier-alias authority contract is definable; implementation should remain in prepared control-flow/publication metadata and tests, not RV64 |

Artifact:
`build/agent_state/464_step1_duplicate_carrier_alias_audit/audit.md`.

## Suggested Next

Execute Step 2 from `plan.md`: define the carrier-alias authority contract,
including required record fields, use-closure requirements, positive/negative
test cases, and the exact future metadata packet files.

Suggested artifact directory:
`build/agent_state/464_step2_carrier_alias_contract/`.

## Watchouts

- Do not edit implementation files during Step 1.
- Do not make RV64 ULE rematerialization or target consumer changes before the
  metadata exists.
- Do not infer duplicate-carrier authority from `%*.phi.sel*` spelling, raw
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
