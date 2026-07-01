Status: Active
Source Idea Path: ideas/open/489_bir_dynamic_local_array_proof_population_from_lir_coordinates.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route Proof Population

# Current Packet

## Just Finished

Completed Step 3 as a route-only packet for idea 489.

Targeted inspection found candidate coordinate and proof-source surfaces, but
not the authoritative producer families required by the Step 2 contract.

| Required surface | Step 3 finding |
| --- | --- |
| `lir_producer_*` coordinate binding | Present through idea 488 LIR producer-site fields and `lir-producer:` lookup keys. |
| Prepared branch/compare proof source | Candidate `FusedCompare` branch-condition facts and operand linkage exist. |
| Proof-source path/dominance coverage | Missing durable certificate keyed to `lir_producer_*`, proof edge/outcome, and covered/guarded path. |
| Dynamic-index same-value/no-clobber interval | Missing durable certificate for redefinition, phi/alias, call/helper, inline-asm, publication/move-bundle, and parallel-copy effects. |
| Idea 486 checker wiring | Present as a checker/status consumer for complete facts; not a real producer. |
| Step 3 decision | No implementation selected. Route to `dynamic local-array LIR producer path/no-clobber certificate`. |

Supporting artifact:

- `build/agent_state/489_step3_proof_population_route/route.md`

## Suggested Next

Execute Step 4 residual disposition for idea 489, recording close/split readiness
around the lower owner `dynamic local-array LIR producer path/no-clobber
certificate`.

## Watchouts

- Do not mark real dynamic rows available until explicit proof-source,
  path/dominance, and same-value/no-clobber facts are accepted by the idea 486
  checker.
- Do not infer proof facts from loop shape, branch proximity, value names,
  testcase names, dump order, final homes, or target behavior.
- Keep `lir_producer_instruction_index` as an LIR producer-site coordinate; do
  not reinterpret it as a prepared traversal or BIR `Block::insts` coordinate.
- Keep idea 486 checker vocabulary, idea 488 coordinate semantics, idea 484
  packaging, scalar local-load consumption, and RV64/MIR lowering out of scope.
- Existing untracked `review/*.md` files were not touched.

## Proof

Step 3 route-only validation:

```sh
git diff --check
```

Result: passed.
