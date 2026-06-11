# Return-Chain Import And Naming

Status: import note complete for Phase E readiness analysis.

## Source Materials

Future plans should cite this note together with:

- `docs/bir_prealloc_fusion/return_chain_owner_schema_decision.md`
- `ideas/closed/176_return_chain_owner_schema_decision.md`
- `ideas/closed/177_bir_return_chain_schema_index.md`
- `ideas/closed/178_bir_return_chain_oracle_equivalence.md`
- `ideas/closed/179_bir_return_chain_consumer_migration.md`
- `ideas/closed/180_bir_return_chain_prepared_api_contraction.md`

Those sources establish a completed return-chain owner/schema line. They do
not establish broader `PreparedFunctionLookups` contraction,
`PreparedBirModule` retirement, draft 155 readiness, x86/riscv wrapper
readiness, or target policy migration.

## Durable Name

The accepted durable name is `Route 8 return-chain owner/schema line`.
Acceptable short forms are `return-chain owner/schema line` when the route
number is not the point, and `Route 8 return-chain line` in compact tables.

Do not cite the line as Route 1 producer identity, Route 7 comparison or
condition provenance, predecessor rescan cleanup, name matching, or generic
route-index progress.

## Target-Neutral Facts

Route 8 owns target-neutral BIR identity for same-function, same-block scalar
binary chains that end at a BIR return terminator. The accepted fact shape is:

- owning function and block context
- binary instruction position or equivalent stable instruction reference
- named chain value produced by that instruction
- named terminal value consumed by the return terminator
- optional named other operand of the immediate next binary in the same chain

The route remains fail-closed for unsupported opcodes, unnamed values, broken
same-block walks, non-return terminators, cross-block relationships, missing
keys, and conflicting duplicate publications. Conflict handling must observe no
usable answer rather than choosing a winner through traversal order, target
state, or consumer preference.

## Target-Local Policy

AArch64 and other target lowering retain all target policy that uses Route 8
answers:

- value-home lookup and missing-home behavior
- ABI return placement and before-return move lookup
- target register parsing or conversion
- alias detection between next operand and selected result registers
- scalar register view and reserved scratch selection
- final ALU or return record construction
- final emission order

Route 8 may supply the semantic terminal or next-operand identity. It must not
store or imply homes, registers, ABI destinations, scratch choices, final
machine records, or ordering policy.

## Prepared Helper And Oracle Status

Closed ideas 176-179 used public prepared return-chain helpers as historical
decision, oracle, and migration surfaces. Idea 178 proved BIR/prepared
equivalence, and idea 179 migrated AArch64 semantic consumers to the BIR-owned
route while preserving target-local policy.

Closed idea 180 then removed the public prepared return-chain helper surface.
Current `src` and `tests` searches show no matches for these removed public
names:

- `PreparedReturnChainLookups`
- `find_prepared_return_chain_terminal_value`
- `find_prepared_return_chain_next_operand_value`
- `prepared_return_chain_value_key`

Future docs should therefore describe the prepared helpers as historical
oracles and migration scaffolding, not as retained public blockers. Current
Route 8 source and tests use `route8_*` BIR records, indexes, and lookup
helpers instead.

## Readiness Analysis Use

For `PreparedFunctionLookups` readiness, cite Route 8 as one completed
target-neutral return-chain identity route whose old public prepared helper
surface is absent after idea 180. This supports the statement that the
return-chain lookup group no longer needs a public prepared helper oracle.
It does not prove that call, addressing, move, value-home, publication,
source-producer, memory, comparison, or other lookup groups are ready to
contract.

For `PreparedBirModule` readiness, cite Route 8 as imported source evidence
for return-chain identity only. It can help separate BIR-owned return-chain
identity from target-owned return ABI, value homes, registers, alias handling,
scratch selection, ALU records, and emission order. It does not make the
prepared module aggregate ready to retire and does not open draft 155 by
itself.

## Citation Rules

Future plans may cite Route 8 for:

- terminal return-chain value identity
- immediate next-operand identity in a same-block return chain
- fail-closed same-block return-chain schema and index behavior
- the historical prepared-oracle sequence through ideas 176-180
- the absence of the old public prepared return-chain helper API after idea 180

Future plans must not cite Route 8 as:

- Route 1 producer identity progress
- Route 7 comparison, condition, branch, or materialized-condition progress
- predecessor-rescan cleanup
- name-matching evidence
- generic route-index facade completion
- x86 or riscv wrapper readiness
- broad `PreparedFunctionLookups` contraction
- broad `PreparedBirModule` retirement
- draft 155 readiness
- target policy migration

When a future analysis needs target behavior, it must name the target-specific
consumer and proof route separately from this Route 8 owner/schema line.
