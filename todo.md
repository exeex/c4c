# Current Packet

Status: Active
Source Idea Path: ideas/open/703_bir_mir_contract_abstraction_umbrella.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh the dependency inventory

## Just Finished

- Completed `plan.md` Step 1: created the reproducible current route-numbered
  dependency inventory, classified BIR, prealloc/prepared, MIR/target, and test
  dependencies by consumer/current producer and semantic vs debug/proof vs
  compatibility, and reconciled stale prepared-MIR-view and route-retirement
  evidence.

## Suggested Next

- Delegate `plan.md` Step 2 to define ownership and named handoff contracts from
  `docs/bir_mir_contract_abstraction/01_current_dependency_inventory.md`.

## Watchouts

- Keep this umbrella docs-and-ideas only; do not modify implementation or tests.
- Leave ideas 647 and 655 parked until explicit positive prepared/prealloc
  producer evidence exists.
- The first prepared MIR view and x86 guard did not migrate common MIR query,
  AArch64, or RV64 route dependencies; do not treat old view research as current
  completion evidence.

## Proof

- Source scan passed after omitting the nonexistent `test/` root; exact command:
  `rg -n "route[1-8]_|Route[1-8]|bir_route[1-8]|RouteIndex|route_index" src/backend/bir src/backend/prealloc src/backend/mir tests`.
  It produced 5,339 lines across 56 files (13 BIR, 6 prealloc, 20 MIR/target,
  17 tests).
- `git diff --check` passed.
- The delegated packet forbade changes to `test_after.log`, so this docs-only
  scan proof is recorded here rather than in a proof log.
