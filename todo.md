# Current Packet

Status: Active
Source Idea Path: ideas/open/703_bir_mir_contract_abstraction_umbrella.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define ownership and named handoff contracts

## Just Finished

- Completed `plan.md` Step 2: classified dependencies by first owner and
  defined narrow named BIR, prepared/prealloc, MIR, target, proof, and
  compatibility contracts, including public/private header rules and the
  positive stack-authority producer gate for ideas 647 and 655.

## Suggested Next

- Delegate `plan.md` Step 3 to generate a dependency-ordered, single-owner
  follow-up idea queue from the inventory and named contracts.

## Watchouts

- Keep this umbrella docs-and-ideas only; do not modify implementation or tests.
- Leave ideas 647 and 655 parked until explicit positive prepared/prealloc
  producer evidence exists.
- Keep BIR view production, prepared fact production, common MIR migration,
  target materialization, route quarantine, and test vocabulary cleanup in
  separate follow-ups; a renamed full route record is not a named contract.
- Stack-authority follow-ups must prove a positive unique prepared producer and
  fail-closed MIR consumption before ideas 647 or 655 can resume.

## Proof

- Delegated source-presence guard passed:
  `rg -n "route[1-8]_|Route[1-8]|bir_route[1-8]|RouteIndex|route_index" src/backend/bir src/backend/prealloc src/backend/mir tests >/dev/null`.
- `git diff --check` passed.
- The delegated packet forbade changes to `test_after.log`, so this docs-only
  proof is recorded here rather than in a proof log.
