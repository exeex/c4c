# Current Packet

Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Repair B1-B2 canonical pass identifiers

## Just Finished

- Plan Step 4 repaired the obsolete stage identifiers across B1 / P01 legalize,
  B2 / P02 scalar canonicalization, and their `ComparisonSelect` analysis
  dependency.
- Recorded the canonical A2 -> B1 / P01 -> B2 / P02 -> B3 / P03 adjacency
  without changing the target-independent transactional contracts, opaque
  `InlineAsm` preservation, or exact-revision immutable analysis boundary.

## Suggested Next

- Execute Plan Step 5, "Repair B3-B4 canonical pass identifiers."

## Watchouts

- Preserve the root README as the sole normative order authority while Step 5
  repairs CFG/SSA pass and analysis/publication identifiers.
- Keep pass-local semantics intact; this packet changed registry identities
  and adjacency only.
- Step 14 and implementation remain forbidden pending completion of the repair
  route and a new blocker-free independent Step 13 review.

## Proof

- Passed: `git diff --check && ! rg -n 'S0[0-9]|S1[0-9]|S2[0-9]|G01' src/backend/bir/passes/legalize/README.md src/backend/bir/passes/scalar/README.md src/backend/bir/analysis/comparison/README.md && rg -n 'A2|B1|B2|B3|P01|P02|target-independent|transaction|revision|opaque' src/backend/bir/passes/legalize/README.md src/backend/bir/passes/scalar/README.md src/backend/bir/analysis/comparison/README.md`.
- The supervisor-selected documentation proof was sufficient; this packet did
  not create or modify `test_after.log` because regression logs were explicitly
  outside packet ownership.
