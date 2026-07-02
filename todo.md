Status: Active
Source Idea Path: ideas/open/523_bir_route2_select_chain_body_extraction.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit route2 symbols and route6 consumers

# Current Packet

## Just Finished

Lifecycle activation created the active runbook for Step 1 of `plan.md`.

## Suggested Next

Execute Step 1: audit route2 symbols and route6 consumers before selecting the extraction boundary.

## Watchouts

- Preserve public route2 declarations and record types in `bir.hpp`.
- Do not let route6 reach into private route2 implementation details.
- Do not change direct-global dependency classification, route6 publication policy, call ABI behavior, tests, or expectations.

## Proof

Activation-only lifecycle change. Run `git diff --check -- plan.md todo.md`.
