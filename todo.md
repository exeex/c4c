Status: Active
Source Idea Path: ideas/open/108_prepared_select_chain_dump_contract_coverage.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Lookup Facts And Existing Dump Coverage

# Current Packet

## Just Finished

Activation created the active runbook and executor scratchpad for Step 1.

## Suggested Next

Execute `plan.md` Step 1: inventory scalar select-chain materialization and
direct-global select-chain dependency lookup facts, compare them against
prepared-printer output, and identify focused prepared-printer test coverage.
Record findings here and keep implementation files unchanged for the inventory
packet.

## Watchouts

- Do not rewrite prepared lookup APIs or target lowering policy.
- Keep source-producer kind/block/index tied to select-chain or direct-global
  dependency visibility.
- Do not create compare-join continuation or select-materialization
  join-transfer work.

## Proof

Activation is lifecycle-only. Run `git diff --check -- plan.md todo.md` for the
activation proof.
