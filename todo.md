Status: Active
Source Idea Path: ideas/open/490_dynamic_local_array_lir_producer_path_no_clobber_certificate.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Path/No-Clobber Certificate Contract

# Current Packet

## Just Finished

Completed Step 2 contract definition for idea 490.

Contract result:

| Area | Step 2 contract |
| --- | --- |
| Certificate key | Key records by `lir_producer_function_name`, `lir_producer_block_label`, `lir_producer_instruction_index`, `lir_producer_operation_role`, `lir_producer_lookup_key`, path identity, dynamic index, and proof branch/compare identity. |
| Accepted representative | Scalar dynamic local-array address-derivation path with exactly one dynamic index, available `lir_producer_*` coordinate, and same-function prepared fused branch/compare proof source. |
| Bound contributions | Require explicit selected edge/outcome, operand role, normalized lower `0` inclusive proof, and upper `element_count` exclusive proof. |
| Path/order | Require explicit path coverage and dominance/guard validity; same-block acceptance needs a truthful ordering proof and must not reinterpret the LIR index as a prepared/BIR instruction index. |
| No-clobber interval | Require dynamic-index same-value proof plus classified redefinition, phi/alias, call/helper, inline asm, publication/move-bundle, and parallel-copy effects. |
| Statuses | Preserve fail-closed statuses for missing/non-covering/non-dominating path, missing ordering, coordinate confusion, clobbered/redefined index, unresolved alias, unknown effects, raw-shape-only, target/final-home-only, and unsupported boundaries. |
| Downstream boundary | Later idea 489 proof population may map an available certificate into idea 486 checker inputs; idea 490 must not populate checker inputs directly. |
| Step 3 decision | Implement only if truthful producers exist for both path/order and interval effects; otherwise route to `dynamic local-array LIR producer interval effect classifier` or a more precise lower owner. |

Supporting artifact:

- `build/agent_state/490_step2_lir_producer_certificate_contract/contract.md`

## Suggested Next

Execute Step 3: inspect for truthful producers for selected proof-source
path/order facts and dynamic-index interval effects. Implement only if both
exist; otherwise route to the exact lower owner without code changes.

## Watchouts

- Do not populate idea 486 checker inputs directly in idea 490.
- Do not infer path coverage or no-clobber from branch proximity, loop shape,
  value names, testcase names, dump order, final homes, or target behavior.
- Keep `lir_producer_instruction_index` as a LIR producer-site coordinate; do
  not reinterpret it as a prepared traversal or BIR `Block::insts` coordinate.
- Same-block rows need a truthful ordering proof or must remain unavailable.
- Existing untracked `review/*.md` files were not touched.

## Proof

Step 2 contract validation:

```sh
git diff --check
```

Result: passed.
