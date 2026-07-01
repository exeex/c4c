Status: Active
Source Idea Path: ideas/open/491_dynamic_local_array_selected_proof_edge_path_certificate.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Selected Proof-Edge Path Certificate Inputs

# Current Packet

## Just Finished

Step 1 audited the current prepared/BIR surfaces for a bounded selected
proof-edge path certificate keyed to dynamic local-array `lir_producer_*`
records.

| Area | Step 1 classification |
| --- | --- |
| Prepared branch/compare proof source | Structured identity exists through `PreparedBranchCondition` and fused compare operand producer facts. |
| Selected edge/outcome | True/false successor labels exist, but no durable record selects one edge/outcome as the proof edge for a `lir_producer_lookup_key`. |
| Path helpers | Prepared dominance/reachability helper logic exists, but it is helper-local evidence, not a durable certificate surface. |
| `lir_producer_*` matching | Local-array path records provide stable LIR producer function/block/index/role/key fields suitable for matching. |
| Same-block ordering | Fail closed unless a separate truthful bridge exists; the LIR producer instruction index is not a prepared/BIR instruction index. |
| Step 2 readiness | A bounded contract can be defined now for cross-block selected proof-edge path certification. |

Artifact: `build/agent_state/491_step1_selected_proof_edge_path_audit/audit.md`.

## Suggested Next

Execute Step 2: define the selected proof-edge path certificate contract. The
contract should require a dynamic local-array `lir_producer_lookup_key`, explicit
prepared branch/compare proof-source identity, selected true/false outcome,
edge tuple, path coverage, dominance/guard validity, and producer-specific
unavailable statuses. Same-block ordering should remain unavailable unless a
truthful coordinate bridge is provided.

## Watchouts

- Keep dynamic-index interval effect/no-clobber classification out of this
  runbook; it is a later owner after selected path certification.
- Do not populate idea 489 proof facts or idea 486 checker inputs directly.
- Keep `lir_producer_instruction_index` as a LIR producer-site coordinate.
- Do not treat helper-level reachability/dominance queries as durable
  certificate records until a Step 3 packet publishes that surface explicitly.
- Do not infer proof edges or path coverage from branch proximity, loop shape,
  value names, testcase names, dump order, final homes, or target behavior.
- Existing untracked `review/*.md` files are transient and must remain
  untouched.

## Proof

Step 1 validation:

```sh
git diff --check
```

Result: passed.
