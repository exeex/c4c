Status: Active
Source Idea Path: ideas/open/466_representative_select_carrier_alias_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Representative Authority Evidence Contract

# Current Packet

## Just Finished

Completed Step 2 contract definition for idea 466. The next packet is justified
as a prepared producer/printer/probe evidence surface, not RV64 lowering.

Required evidence surface:

| Row type | Required facts | Purpose |
| --- | --- | --- |
| Available authority record | Function, predecessor, successor/join label, destination value/id/name, selected source value/id/name, source producer kind/block/instruction, join-transfer carrier kind, carrier aliases with value/id/name/block/instruction, and `source_use_closure_proven`. | Proves a record exists and can be compared with RV64 matcher keys. |
| Unavailable candidate/status row | Function, edge, destination/source ids and names, publication status, carrier kind, source producer fields, candidate alias count/names where available, and `PreparedSelectCarrierAliasAuthorityStatus`. | Distinguishes missing producer record from hidden record and names exact rejection status. |
| Representative positive target | `20010329-1` `main`, `logic.rhs.end.40 -> logic.end.41`, `%t46 -> %t50`, binary `ule ptr` source producer, aliases `%t50.phi.sel0/%t50.phi.sel1`. | Lets idea 465 resume only if a matching record is proven present. |

Positive cases: available duplicate-carrier authority for a representative-style
select-materialization edge with binary source producer, matching final carrier,
carrier aliases, and proven source-use closure.

Negative cases: missing source producer, unsupported publication, wrong carrier
kind, non-binary producer, missing final carrier, missing/duplicate/unsupported
aliases, mismatched carrier alias, non-carrier source use, wrong edge, wrong
destination/source, and wrong producer coordinate.

Artifact:
`build/agent_state/466_step2_representative_authority_evidence_contract/contract.md`.

## Suggested Next

Execute Step 3: implement or route the focused representative authority
evidence packet. Suggested owned files are `publication_plans.hpp/.cpp`,
prepared-printer surfaces, focused BIR/prepared-printer tests, `todo.md`,
`test_after.log`, and
`build/agent_state/466_step3_representative_authority_evidence/*`.

Step 3 proof:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Do not classify the representative as solved from raw duplicate-carrier
  select shape.
- Do not claim the record is missing or hidden until a focused evidence surface
  can observe record count and/or rejection status.
- Unavailable status rows are diagnostics only; they are not RV64 authority.
- Do not route back to RV64 consumer work unless the record is proven present
  with fields that RV64 mismatches.
- Do not make RV64 ULE rematerialization changes until representative
  authority is proven present and matchable.
- Do not infer aliases from `%*.phi.sel*` spelling, raw select shape, value
  ids, block labels, function names, testcase names, or dump order.
- Do not implement plain `%t46 -> %t50` copies or same-register no-ops.
- Do not add generic stack-to-register, register-to-register, or all-purpose
  move-bundle support.
- Do not consume `load_from_stack_slot missing_stack_freshness`.
- Do not modify `test_baseline.new.log`, `test_baseline.log`,
  `test_before.log`, `test_after.log`, or `review/`.

## Proof

Step 2 proof:

```sh
git diff --check
```

Result: passed.
