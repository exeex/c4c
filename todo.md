Status: Active
Source Idea Path: ideas/open/197_return_chain_import_and_naming_clarification.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Define Naming and Future Citation Rules

# Current Packet

## Just Finished

Step 3 naming and future citation rules are complete. The accepted durable
name is `Route 8 return-chain owner/schema line`; acceptable short forms are
`return-chain owner/schema line` when the route number is not the focus, and
`Route 8 return-chain line` in compact tables.

Future `PreparedFunctionLookups` readiness analysis should cite this line as
one completed target-neutral return-chain identity route. It may say that the
old public prepared return-chain lookup/helper surface is absent after idea
180, and that current AArch64 return-chain lowering reads BIR Route 8 answers
plus target/prepared materialization data. It must not treat Route 8 as proof
that other `PreparedFunctionLookups` groups are ready to contract.

Future `PreparedBirModule` readiness analysis should cite this line as imported
source evidence for return-chain identity only. It may use the decision doc and
closed ideas 176-180 to separate BIR-owned identity from target-owned return
ABI, value homes, registers, alias handling, scratch selection, ALU records,
and emission order. It must not treat Route 8 as broad module-retirement or
draft-155 readiness.

Negative citation rules:

| Do not cite Route 8 as... | Required replacement wording |
| --- | --- |
| Route 1 producer identity progress | Route 8 answers terminal return-chain and optional immediate next-operand identity; Route 1 remains source-producer identity. |
| Route 7 comparison/condition progress | Route 8 is not comparison provenance, materialized-condition provenance, or branch-condition evidence. |
| Predecessor rescan cleanup | The line is a function/block/instruction/value-keyed BIR route, not a fresh predecessor search. |
| Name matching | Stable names participate in route keys and answers, but the accepted fact is the same-block return-chain relation, not string equality. |
| Generic route-index progress | Route 8 has a distinct schema and index; do not flatten it into route-index facade completion. |
| Retained public prepared helper evidence | Ideas 176-179 used public prepared helpers historically, but idea 180 removed the public helper surface. Current `src`/`tests` searches show `PreparedReturnChainLookups`, `find_prepared_return_chain_terminal_value`, `find_prepared_return_chain_next_operand_value`, and `prepared_return_chain_value_key` absent. |
| x86/riscv wrapper readiness | The return-chain line proves no x86 or riscv wrapper migration boundary. Cite target-specific wrapper readiness separately. |
| Broad aggregate/module retirement readiness | The line resolves one return-chain owner/schema import ambiguity only. It is not `PreparedFunctionLookups` contraction or `PreparedBirModule` retirement evidence by itself. |

## Suggested Next

Execute Step 4: write `docs/bir_prealloc_fusion/return_chain_import_and_naming.md`
as the durable import note. The note should cite the decision doc and closed
ideas 176-180, use the accepted `Route 8 return-chain owner/schema line` name,
separate target-neutral identity from target-local AArch64/prepared policy,
and include the negative citation rules above.

## Watchouts

- Keep the status time-aware. Closed ideas 176-179 and the decision doc used
  public prepared helpers as oracle and migration surfaces; closed idea 180
  removed the public helper API, and current source/test searches show it is
  absent.
- Do not let Step 4 revive stale Phase C wording that says public prepared
  return-chain helpers remain retained blockers; cite that only as historical
  pre-contraction context.
- Do not import AArch64 target policy into the route line. ABI moves, homes,
  registers, alias checks, scratch choice, final ALU records, and emission
  order remain lowering policy.
- Do not use this naming packet to edit implementation, tests, expectations,
  unsupported markers, source ideas, closed ideas, or `plan.md`.

## Proof

Docs/lifecycle-only naming and citation packet. Read active `plan.md`, source
idea 197, current `todo.md`, the return-chain decision doc, closed ideas
176-180, and current readiness docs under `docs/bir_prealloc_fusion`. Ran
targeted `rg` scans for `Route 8`, `return-chain`, `PreparedFunctionLookups`,
and `PreparedBirModule` across the return-chain docs/ideas/readiness docs. Ran
current source/test searches for `PreparedReturnChainLookups`,
`find_prepared_return_chain_terminal_value`,
`find_prepared_return_chain_next_operand_value`, and
`prepared_return_chain_value_key`; no matches were found under `src` or
`tests`. No build/test subset was required and no `test_after.log` was
generated.
