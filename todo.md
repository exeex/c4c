Status: Active
Source Idea Path: ideas/open/463_select_edge_ule_source_producer_rematerialization.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Residual Disposition And Close Readiness

# Current Packet

## Just Finished

Completed Step 4 residual disposition for idea 463. The idea is close-ready as
a negative route-classification slice: no RV64 ULE rematerialization packet is
selected until a separate producer/prepared carrier-alias metadata initiative
publishes the missing duplicate-carrier authority.

Residual disposition table:

| Field | Value |
| --- | --- |
| `%t46 -> %t50` plain predecessor-edge copy | Rejected by idea 462 and still out of scope because `%t46` is successor-defined |
| `%t46 = bir.ule ptr %t42, %t45` rematerialization | Blocked before RV64 consumer; requires explicit carrier-alias metadata |
| `%t42` / `%t45` operands | Register homes are not the first blocker for this route |
| `%t50.phi.sel0` / `%t50.phi.sel1` | Exact missing fact is durable prepared metadata proving both are carrier-only aliases of final join-transfer result `%t50` |
| Follow-up owner | Prepared control-flow/publication metadata for select-edge source-producer carriers |
| Candidate surfaces | `src/backend/prealloc/control_flow.hpp`, `src/backend/prealloc/prepared_object_traversal.*`, `src/backend/prealloc/select_chain_lookups.*`, `src/backend/prealloc/publication_plans.*`, focused `tests/backend/bir/` coverage |
| Lifecycle recommendation | Close idea 463 or split a new producer/prepared carrier-alias metadata idea; do not keep this plan active for RV64 lowering |

Artifact:
`build/agent_state/463_step4_residual_disposition/disposition.md`.

## Suggested Next

Plan-owner review: close idea 463 as a completed negative classification slice
or split a new producer/prepared metadata source idea for duplicate select
carrier-alias authority. Return to RV64 ULE rematerialization only after that
metadata exists.

Suggested artifact directory:
`build/agent_state/463_step4_residual_disposition/`.

## Watchouts

- Do not edit RV64 implementation files until explicit carrier-alias metadata
  exists.
- Do not keep idea 463 active for an RV64 packet without that metadata.
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

Step 4 proof:

```sh
git diff --check
```

Result: passed.
