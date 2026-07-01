Status: Active
Source Idea Path: ideas/open/491_dynamic_local_array_selected_proof_edge_path_certificate.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route Selected Edge Path Certificate

# Current Packet

## Just Finished

Step 3 inspected the current prepared/BIR surfaces against the selected
proof-edge path certificate contract and routed instead of implementing.

| Area | Step 3 evidence |
| --- | --- |
| `lir_producer_*` matching | Local-array path records provide stable keys, but do not select a proof edge or prove coverage. |
| Branch/compare records | Prepared branch conditions expose proof-source identity and true/false labels, but no selected outcome record keyed to a local-array producer. |
| Path helpers | Prepared reachability/dominance helpers exist only as internal helper logic, not a durable certificate surface. |
| Existing checkers | Range-proof and interval/source-fact surfaces consume path-validity booleans; they do not produce selected proof-edge path certificates. |
| Same-block ordering | No truthful ordering bridge exists between LIR producer indexes and prepared/BIR instruction coordinates. |
| Outcome | Implementation would require inference, so the packet routes to `dynamic local-array selected proof-edge path record/status API`. |

Artifact: `build/agent_state/491_step3_selected_edge_path_route/route.md`.

## Suggested Next

Execute Step 4: record residual disposition for idea 491. Classify it as a
routed blocker unless lifecycle review chooses to split/activate the exact next
owner, `dynamic local-array selected proof-edge path record/status API`.

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
- The range-proof checker takes `path_validity_known`,
  `proof_dominates_consumer`, and `path_covers_consumer` as inputs; do not use
  it as the producer for those facts.
- Existing untracked `review/*.md` files are transient and must remain
  untouched.

## Proof

Step 3 validation:

```sh
git diff --check
```

Result: passed.
