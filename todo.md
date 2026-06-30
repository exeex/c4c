Status: Active
Source Idea Path: ideas/open/468_carrier_alias_identity_publication_api.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Carrier Alias Identity Callers

# Current Packet

## Just Finished

Parked idea 467 after Step 3 exposed a caller/API boundary instead of an
acceptance-ready planner repair. The attempted hidden `const_cast` mutation is
rejected, and the const-correct scratch-copy route is insufficient because
original-module consumers do not share the synthesized alias identity.

The new active idea 468 owns the prerequisite identity publication boundary:

| Field | Value |
| --- | --- |
| Synthesized aliases | `%t50.phi.sel0` / `%t50.phi.sel1` |
| Needed fact | Shared durable `ValueNameId` identity for carrier aliases after semantic candidate checks pass |
| Rejected route | Hidden mutation behind `const PreparedNameTables&` |
| Insufficient route | Scratch-copy collector publication not visible to original-module consumers |
| Acceptable routes | Explicit mutable pre-consumer publication stage, or consumer-facing API that does not rely on scratch-copy name insertion |

## Suggested Next

Execute Step 1 from `plan.md`: audit carrier-alias collection callers,
prepared dump paths, and RV64/object-route prepared-module access. Produce a
caller matrix showing which consumers need shared identity and where the
publication/API boundary can live.

Suggested artifact directory:
`build/agent_state/468_step1_carrier_alias_identity_callers/`.

## Watchouts

- Do not edit implementation files during Step 1.
- Do not reintroduce `const_cast` or hidden mutation.
- Do not claim scratch-copy-only publication is sufficient for
  original-module consumers.
- Do not make RV64 lowering changes in this identity/API idea.
- Do not infer aliases from raw `%*.phi.sel*` spelling, raw select shape,
  value ids, block labels, function names, testcase names, or dump order.
- Do not modify `test_baseline.new.log`, `test_baseline.log`,
  `test_before.log`, `test_after.log`, or `review/`.

## Proof

Lifecycle split proof:

```sh
git diff --check
python3 scripts/plan_review_state.py show
```

Result: passed.
