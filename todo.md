Status: Active
Source Idea Path: ideas/open/463_select_edge_ule_source_producer_rematerialization.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route First Rematerialization Packet

# Current Packet

## Just Finished

Completed Step 3 for idea 463 as a routing/blocker packet. No RV64
rematerialization implementation is selected because the required duplicate
carrier-alias metadata is not present. The next owner is the producer/prepared
metadata layer that can publish carrier-alias authority for `%t50.phi.sel0` /
`%t50.phi.sel1`.

Route table:

| Field | Value |
| --- | --- |
| RV64 decision | Blocked; do not implement ULE rematerialization from current facts |
| Exact blocker | Missing durable prepared carrier-alias authority proving `%t50.phi.sel0` and `%t50.phi.sel1` are carrier-only aliases of final join-transfer result `%t50` |
| Metadata owner | Prepared control-flow/publication metadata for select-edge source-producer carriers |
| Candidate producer surfaces | `src/backend/prealloc/control_flow.hpp`, `src/backend/prealloc/prepared_object_traversal.*`, `src/backend/prealloc/select_chain_lookups.*`, `src/backend/prealloc/publication_plans.*` |
| Required positive facts | Edge identity, join-transfer result `%t50`, source producer `%t46 = bir.ule ptr %t42, %t45`, duplicate carrier aliases, use closure, and a query key consumable without raw select-shape scanning |
| Required negative tests | Mismatched carrier result, wrong source value or edge, extra non-carrier use, raw-name-only `.phi.sel` shape, missing edge publication, missing source-producer fact |
| Later RV64 boundary | Consume carrier-alias metadata only after it exists; still reject plain copy/no-op, stale stack-load, generic move-bundle, and raw-shape inference |

Artifact:
`build/agent_state/463_step3_select_edge_ule_source_producer_route/route.md`.

## Suggested Next

Execute Step 4 from `plan.md`: residual disposition and close/split readiness.
Record that idea 463 has no exact in-scope RV64 packet until the producer /
prepared carrier-alias metadata initiative exists, then recommend close or
split accordingly.

Suggested artifact directory:
`build/agent_state/463_step4_residual_disposition/`.

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

Step 3 proof:

```sh
git diff --check
```

Result: passed.
