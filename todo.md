# Current Packet

Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Repair B3-B4 canonical pass identifiers

## Just Finished

- Plan Step 5 repaired obsolete stage identifiers across B3 / P03 CFG, B4 /
  P04 SSA, and their CFG, dominance, and publication/value-flow analysis
  dependencies.
- Recorded the canonical B2 / P02 -> B3 / P03 -> B4 / P04 -> B5 / P05
  adjacency without changing explicit-`Phi`, core `EdgeKey`, transactional,
  exact-revision immutable analysis, or D5 deferral semantics.

## Suggested Next

- Execute Plan Step 6, "Repair B5-B8 identifiers and Canonical publication."

## Watchouts

- Preserve the root README as the sole normative order authority while Step 6
  repairs the remaining canonical pass and verifier identifiers.
- Keep B3 / P03 terminator and exact-edge semantics, B4 / P04 explicit-`Phi`
  semantics, and the exact D5 phi-materialization deferral intact.
- Step 14 and implementation remain forbidden pending completion of the repair
  route and a new blocker-free independent Step 13 review.

## Proof

- Passed: `git diff --check && ! rg -n 'S0[0-9]|S1[0-9]|S2[0-9]|G01' src/backend/bir/passes/cfg/README.md src/backend/bir/analysis/cfg/README.md src/backend/bir/analysis/dominance/README.md src/backend/bir/passes/ssa/README.md src/backend/bir/analysis/publication/README.md && rg -n 'B2|B3|B4|B5|P03|P04|EdgeKey|Phi|revision|transaction|D5' src/backend/bir/passes/cfg/README.md src/backend/bir/analysis/cfg/README.md src/backend/bir/analysis/dominance/README.md src/backend/bir/passes/ssa/README.md src/backend/bir/analysis/publication/README.md`.
- Passed follow-up: `git diff --check && ! rg -n 'phi or block-argument|block-argument incoming' src/backend/bir/passes/cfg/README.md && rg -n 'explicit.*Phi|phi incoming|EdgeKey' src/backend/bir/passes/cfg/README.md`.
- The supervisor-selected documentation proof was sufficient; this packet did
  not create or modify `test_after.log` because regression logs were explicitly
  outside packet ownership.
