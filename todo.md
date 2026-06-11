Status: Active
Source Idea Path: ideas/open/197_return_chain_import_and_naming_clarification.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Write the Durable Import Note

# Current Packet

## Just Finished

Step 4 durable import note is complete in
`docs/bir_prealloc_fusion/return_chain_import_and_naming.md`.

The note cites `docs/bir_prealloc_fusion/return_chain_owner_schema_decision.md`
and closed ideas 176-180 as the source material chain. It uses the accepted
durable name `Route 8 return-chain owner/schema line`, with short forms for
compact references, and keeps Route 8 separate from Route 1 producer identity,
Route 7 comparison/condition provenance, predecessor rescans, name matching,
and generic route-index facade progress.

The artifact separates target-neutral facts from target-local policy:
Route 8 owns same-function, same-block BIR return-chain identity, terminal
return value identity, optional immediate next-operand identity, and
fail-closed schema/index behavior. AArch64 and target lowering retain value
homes, ABI return placement, register parsing/conversion, alias checks, scalar
register views, scratch selection, final ALU/return records, and emission
order.

The note records the prepared helper/oracle timeline: public prepared
return-chain helpers were historical through ideas 176-179 as decision,
oracle, and migration surfaces; idea 180 removed the public helper API; current
source/test searches show the old public helper names absent. It also states
how future `PreparedFunctionLookups` and `PreparedBirModule` readiness analysis
should cite Route 8 without claiming broad aggregate contraction, broad module
retirement, draft 155 readiness, x86/riscv wrapper readiness, or target policy
migration.

## Suggested Next

Execute Step 5 final consistency check. Verify the new artifact against the
source idea acceptance criteria and reviewer reject signals, confirm the diff
contains only lifecycle/docs changes, and run formatting or whitespace checks
for the changed Markdown files. If clean, recommend the source idea for
plan-owner close.

## Watchouts

- Some older Phase C/D docs still contain stale pre-contraction phrasing about
  public prepared return-chain helpers remaining public blockers. Treat that as
  historical context unless paired with the idea-180 contraction result.
- The Step 5 checker should specifically reject any wording that converts
  Route 8 into Route 1, Route 7, a generic route-index facade, or broad
  `PreparedFunctionLookups`/`PreparedBirModule` readiness.
- This packet did not touch implementation, tests, expectations, unsupported
  markers, source ideas, closed ideas, or `plan.md`; preserve that boundary in
  Step 5.

## Proof

Docs/lifecycle-only artifact packet. Read active `plan.md`, source idea 197,
current `todo.md`, `docs/bir_prealloc_fusion/return_chain_owner_schema_decision.md`,
and closed ideas 176-180. Ran targeted `rg` scans for `Route 8`,
`return-chain`, `PreparedFunctionLookups`, and `PreparedBirModule` across
return-chain docs, current readiness docs, and closed ideas. Ran current
source/test searches for `PreparedReturnChainLookups`,
`find_prepared_return_chain_terminal_value`,
`find_prepared_return_chain_next_operand_value`, and
`prepared_return_chain_value_key`; no matches were found under `src` or
`tests`. No build/test subset was required and no new `test_after.log` was
generated for this packet.
