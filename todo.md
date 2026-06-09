Status: Active
Source Idea Path: ideas/open/145_current_block_join_fact_routing_split.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inspect Current Join-copy Fact and Routing Uses

# Current Packet

## Just Finished

Activation created the runbook from
`ideas/open/145_current_block_join_fact_routing_split.md`; no implementation
packet has run yet.

## Suggested Next

Start with `plan.md` Step 1. Inspect the current join-copy fact/routing symbols,
classify reusable fact ownership versus AArch64 routing convenience, and record
any boundary ambiguity before code edits.

## Watchouts

- Do not treat target routing booleans as shared prealloc policy.
- Do not delete reusable join-copy source facts.
- Do not prove only one dispatch case while leaving nearby join-copy cases
  unexamined.

## Proof

Lifecycle activation only. No build or test proof required yet.
