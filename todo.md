Status: Active
Source Idea Path: ideas/open/142_value_home_move_bundle_lookup_ownership.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Value-Home And Move-Bundle Surface

# Current Packet

## Just Finished

Lifecycle activation created the active runbook from
`ideas/open/142_value_home_move_bundle_lookup_ownership.md`.

## Suggested Next

Execute Step 1 from `plan.md`: map value-home and move-bundle declarations,
definitions, consumers, chosen value-location owners, and the current-block
publication ownership decision before editing code.

## Watchouts

- This active idea is an ownership move only; do not change value-home
  construction, move-bundle semantics, call plans, return handling, or target
  emission policy.
- Keep `PreparedFunctionLookups` as the aggregate wiring point.
- Do not move return-chain lookups.
- Do not replace reusable lookup facts with local rescans.
- Only include current-block entry publication status/query if inspection
  proves it remains directly tied to value homes and block-entry publication
  collection.
- Do not mix in call-argument, comparison, load-local, current-block
  join-routing, or residual include cleanup work; those are separate open
  ideas.
- Keep `prepared_lookups.hpp` includes where a consumer still needs
  `PreparedFunctionLookups`, `make_prepared_function_lookups`, return-chain
  helpers, or another residual prepared lookup API.

## Proof

Activation only; no build proof required yet.
