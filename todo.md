Status: Active
Source Idea Path: ideas/open/144_source_producer_same_block_materialization_owner.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Declaration Surface And Consumers

# Current Packet

## Just Finished

Lifecycle activation created the active runbook from
`ideas/open/144_source_producer_same_block_materialization_owner.md`.

## Suggested Next

Execute Step 1 from `plan.md`: map same-block/source-producer declarations,
definitions, consumers, and selected narrow owners before editing code.

## Watchouts

- This active idea is an ownership move only; do not change source-producer,
  publication, materialization, AArch64 emission, call, comparison, or memory
  behavior.
- Prefer `src/backend/prealloc/select_chain_lookups.hpp` for
  source-producer/same-block materialization declarations unless inspection
  proves a fact struct already belongs in `publication_plans.hpp`.
- Do not replace prepared facts with local AArch64 rediscovery scans.
- Do not mix in call-argument, comparison, load-local, or residual include
  cleanup work; those are separate open ideas.
- Keep `prepared_lookups.hpp` includes where a consumer still needs
  `PreparedFunctionLookups`, `make_prepared_function_lookups`, or another
  residual prepared lookup API.

## Proof

Activation only; no build proof required yet.
