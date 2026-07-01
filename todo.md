Status: Active
Source Idea Path: ideas/open/491_dynamic_local_array_selected_proof_edge_path_certificate.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Residual Disposition And Close Readiness

# Current Packet

## Just Finished

Step 4 recorded residual disposition for idea 491 after Step 3 routed selected
proof-edge path certification to a missing durable record/status API.

| Area | Step 4 disposition |
| --- | --- |
| 491 outcome | Close-ready as a routed blocker investigation, not an implementation slice. |
| Completed value | Audited the current surfaces, defined the selected proof-edge path certificate contract, and proved the missing first owner. |
| Exact next owner | `dynamic local-array selected proof-edge path record/status API`. |
| Required next fields | `lir_producer_*` key, proof branch/compare identity, selected outcome, edge tuple, path coverage, dominance/guard validity, and fail-closed statuses. |
| Later owner | Dynamic-index interval effect/no-clobber classification remains separate after selected path certification exists. |
| Downstream status | Idea 489 proof population, idea 486 checker inputs, idea 484 packaging, scalar local-load consumption, and RV64/MIR remain blocked/out of scope. |

Artifact: `build/agent_state/491_step4_residual_disposition/disposition.md`.

## Suggested Next

Supervisor/plan-owner lifecycle review can close or hand off idea 491 as a
routed blocker and activate the exact next owner:
`dynamic local-array selected proof-edge path record/status API`.

## Watchouts

- Keep dynamic-index interval effect/no-clobber classification out of this
  runbook; it is a later owner after selected path certification.
- Do not populate idea 489 proof facts or idea 486 checker inputs directly.
- Keep `lir_producer_instruction_index` as a LIR producer-site coordinate.
- Same-block proof-source-to-producer candidates remain unavailable unless a
  future packet supplies a truthful ordering bridge.
- Do not treat helper-level reachability/dominance queries as durable
  certificate records until a future owner publishes that surface explicitly.
- Do not infer proof edges or path coverage from branch proximity, loop shape,
  value names, testcase names, dump order, final homes, or target behavior.
- The range-proof checker takes `path_validity_known`,
  `proof_dominates_consumer`, and `path_covers_consumer` as inputs; do not use
  it as the producer for those facts.
- Existing untracked `review/*.md` files are transient and must remain
  untouched.

## Proof

Step 4 validation:

```sh
git diff --check
```

Result: passed.
