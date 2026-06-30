Status: Active
Source Idea Path: ideas/open/467_unsupported_carrier_alias_planner_rejection.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Unsupported Carrier Alias Rejection

# Current Packet

## Just Finished

Completed Step 1 audit for idea 467. The representative
`unsupported_carrier_alias` is caused by the carrier-alias acceptance loop,
not candidate collection.

Rejection audit:

| Predicate / fact | Evidence | Verdict |
| --- | --- | --- |
| Candidate collection | `carrier_alias_candidates=2`; raw BIR has `%t50.phi.sel0` and `%t50.phi.sel1` payload-using `%t46` and feeding final `%t50`. | Passed. |
| Candidate shape checks | Candidates are in successor `logic.end.41`, are named values, are distinct from `%t50`, payload-use `%t46`, do not condition-use `%t46`, and feed final `%t50`. | Passed or implied by collection. |
| Prepared value-name lookup | Planner calls `prepared_named_value_id(*inputs.names, carrier->result)`; prepared dump has homes/storage for `%t50` but no value-id/home rows for `%t50.phi.sel0` or `%t50.phi.sel1`. | First failing predicate. |
| Accepted aliases | Evidence row reports `carrier_aliases=0` and `source_use_closure=no`. | Failure happens before alias records are appended and before closure proof. |

Exact missing fact: durable prepared value-name identity for synthesized
carrier alias select results `%t50.phi.sel0` / `%t50.phi.sel1`.

Artifact:
`build/agent_state/467_step1_unsupported_carrier_alias_audit/audit.md`.

## Suggested Next

Execute Step 2: define the carrier-alias acceptance contract. The contract
should specify how synthesized carrier alias result names become
producer-published prepared identity, whether alias value ids/homes are
optional or required, and the fail-closed cases for unpublished aliases, wrong
final result, wrong source/edge, duplicate candidates, and extra source uses.

## Watchouts

- Do not treat candidate discovery as authority; candidates still require
  prepared identity and closure proof.
- Do not intern or accept `.phi.sel` names merely because they match the raw
  spelling pattern.
- Do not treat unavailable evidence rows as authority.
- Do not make RV64 ULE rematerialization changes until an available authority
  record exists or a later packet proves an RV64 matcher mismatch against one.
- Do not infer aliases from `%*.phi.sel*` spelling, raw select shape, value
  ids, block labels, function names, testcase names, or dump order.
- Do not implement plain `%t46 -> %t50` copies or same-register no-ops.
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
