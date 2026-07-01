Status: Active
Source Idea Path: ideas/open/491_dynamic_local_array_selected_proof_edge_path_certificate.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Selected Proof-Edge Path Certificate Contract

# Current Packet

## Just Finished

Step 2 defined the selected proof-edge path certificate contract for dynamic
local-array `lir_producer_*` records.

| Area | Step 2 contract |
| --- | --- |
| Certificate key | Requires exact `lir_producer_*` fields, `lir_producer_lookup_key`, proof branch/compare identity, selected outcome, and proof-block -> selected-successor edge tuple. |
| Accepted representative | Cross-block dynamic local-array address derivation guarded by an explicit prepared fused branch/compare selected true/false edge. |
| Path coverage | Selected successor must reach and cover the LIR producer block through durable certificate facts, not route-local helper inference. |
| Dominance/guard validity | Proof source must dominate or guard the producer site under the selected outcome. |
| Same-block policy | Explicitly fail closed without a truthful ordering bridge; never compare LIR producer index to prepared/BIR instruction index. |
| Rejected shapes | Missing proof source/edge/outcome, non-covering path, non-dominating proof, unsupported boundary, coordinate confusion, raw-shape-only, target/final-home-only. |

Artifact: `build/agent_state/491_step2_selected_proof_edge_path_contract/contract.md`.

## Suggested Next

Execute Step 3: inspect whether existing prepared path helpers and
branch/compare records can publish a truthful durable selected proof-edge path
record/status surface keyed to `lir_producer_lookup_key`. Implement only if the
surface can expose selected outcome, edge tuple, path coverage, and
dominance/guard validity without inference; otherwise route to the exact
path-certificate API blocker.

## Watchouts

- Keep dynamic-index interval effect/no-clobber classification out of this
  runbook; it is a later owner after selected path certification.
- Do not populate idea 489 proof facts or idea 486 checker inputs directly.
- Keep `lir_producer_instruction_index` as a LIR producer-site coordinate.
- Same-block proof-source-to-producer candidates remain unavailable unless a
  future packet supplies a truthful ordering bridge.
- Do not treat helper-level reachability/dominance queries as durable
  certificate records until a Step 3 packet publishes that surface explicitly.
- Do not infer proof edges or path coverage from branch proximity, loop shape,
  value names, testcase names, dump order, final homes, or target behavior.
- Existing untracked `review/*.md` files are transient and must remain
  untouched.

## Proof

Step 2 validation:

```sh
git diff --check
```

Result: passed.
