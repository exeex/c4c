# Current Packet

Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 9
Current Step Title: Converge out-of-SSA

## Just Finished

- Plan Step 9 is complete. D5 consumes only the exact fully reverified D4
  `PseudoBir`, snapshots every phi/block argument, and plans each typed transfer
  by exact `EdgeKey` before mutation.
- Edge-local placement is deterministic: a unique source-successor region, a
  unique destination-incoming region, or a transactional split of the exact
  critical/parallel edge occurrence. Terminators remain the sole CFG truth.
- D5 emits admitted `EdgeCopy` singletons or atomic `ParallelCopy` bundles with
  canonical destination order and simultaneous read-before-write semantics.
  Former join-result IDs remain stable virtual allocation identities; no phi,
  block argument, incoming map, or SSA-only edge-use semantics survive.
- Every mutation advances the revision, invalidates and recomputes affected
  facts, and must pass full-module Pseudo reverification before the D5
  `PseudoBir` capability can reach E1. Failure rolls back the whole candidate;
  MIR owns none of normal out-of-SSA.

## Suggested Next

- Execute Plan Step 10: converge shared revision-bound BIR liveness,
  interference, constraint consumption, and allocation authority across
  E1/E2, using only the fully reverified D5 `PseudoBir`.

## Watchouts

- Do not begin implementation before explicit architecture acceptance.
- E1 must consume the exact D5 revision and its full verifier capability; it
  cannot reuse pre-D5 CFG, dominance, SSA, def-use, liveness, constraints, or
  realizability products merely because stable IDs survived.
- Preserve `ParallelCopy` simultaneous semantics through allocation and MIR
  mapping. Coalescing or concrete move scheduling cannot change edge coverage,
  introduce an implicit phi obligation, or select a different incoming value.
- Step 10 must keep one shared BIR allocation authority; concrete registers,
  frame offsets, target opcodes, and capacity `Spill`/`Reload` remain outside
  E1/E2 ownership as assigned by the root stage order.
- Keep idea 731 open when this docs-only runbook is exhausted.

## Proof

- `git diff --check && ! rg -n 'MIR.*out.of.SSA|out.of.SSA.*MIR|hidden (edge|copy)|label.*edge.*authority|predecessor.*stored.*authority' src/backend/bir/passes/out_of_ssa/README.md src/backend/bir/passes/cfg/README.md src/backend/bir/passes/ssa/README.md src/backend/bir/pseudo/README.md src/backend/bir/verify/README.md && rg -n 'D5|ParallelCopy|EdgeCopy|EdgeKey|critical.edge|revision|invalidat|reverif|rollback|E1' src/backend/bir/passes/out_of_ssa/README.md src/backend/bir/pseudo/README.md src/backend/bir/verify/README.md` — exit 0.
- The supervisor selected a docs-only structural proof that does not produce a
  test log; no `test_after.log` or other regression log was created or
  modified.
