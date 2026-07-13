# Current Packet

Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Converge canonical passes P03-P04 and CFG/SSA analyses

## Just Finished

- Plan Step 5 is complete. P03 now owns all CFG mutation through one function
  transaction, with typed terminators and successor slots as the sole stored
  edge source and `EdgeKey {source BlockId, successor-slot ordinal}` preserving
  split/merge, switch, indirect, asm-goto, and parallel-edge occurrences.
- CFG, dominance, and publication/value-flow are closed immutable analyses
  keyed to exact revisions, with explicit dependencies, stale-handle rejection,
  exact invalidation, atomic cache publication, and no graph mutation authority.
- P04 now has closed phi/block-argument construction and repair semantics:
  incoming values cover the exact live `EdgeKey` multiset, def-use and dominance
  are repaired and verified transactionally, and any incomplete repair rolls
  back the entire occurrence.
- Phi materialization and cycle-safe parallel-copy realization are deferred to
  root stage `D5` after target legalization and before `E1` allocation
  liveness; P04 makes no placement, allocation, or target-instruction decision.
- Minimal pipeline adjacency wording now uses `BlockId`/`EdgeKey` consistently
  and rejects names or label lookup tables as graph keys.

## Suggested Next

- Execute Plan Step 6 in its declared order: converge
  `passes/memory/README.md` (`P05`), `analysis/memory_effects/README.md`,
  `analysis/provenance/README.md`, `passes/aggregate/README.md` (`P06`),
  `passes/intrinsics/README.md` (`P07`), `analysis/call_graph/README.md`, then
  the Canonical profile in `verify/README.md`. Require the exact P07 output to
  pass Canonical publication, keep all facts target-independent, and preserve
  inline assembly as one opaque semantic node.

## Watchouts

- Do not begin implementation before explicit architecture acceptance.
- Do not let a local pass or analysis document invent ordering outside the root
  README or treat an analysis dependency as a serial stage.
- P05-P07 must preserve the P03/P04 CFG and SSA profiles or fail; a later
  canonicalizer may not emit a fresh noncanonical scalar, edge, phi, memory, or
  aggregate form and then rerun an earlier pass.
- Do not move D5 work into P04 or a target instruction graph. Preparation and
  allocation remain outside the canonical P05-P07 interval.
- Keep idea 731 open when this docs-only runbook is exhausted.

## Proof

- `git diff --check && ! rg -n 'persistent predecessor|predecessor.*authority|MIR.*out.of.SSA|out.of.SSA.*MIR|label.*identity' src/backend/bir/passes/cfg/README.md src/backend/bir/analysis/cfg/README.md src/backend/bir/analysis/dominance/README.md src/backend/bir/passes/ssa/README.md src/backend/bir/analysis/publication/README.md src/backend/bir/pipeline/README.md && rg -n 'terminator|EdgeKey|revision|invalidat|D5|rollback|publish' src/backend/bir/passes/cfg/README.md src/backend/bir/analysis/cfg/README.md src/backend/bir/analysis/dominance/README.md src/backend/bir/passes/ssa/README.md src/backend/bir/analysis/publication/README.md` — exit 0.
- The supervisor selected a docs-only structural proof that does not produce a
  test log; no `test_after.log` or other regression log was created or
  modified.
