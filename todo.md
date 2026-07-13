# Current Packet

Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 9.1
Current Step Title: Choose the BIR-owned D5 parallel-copy realization route

## Just Finished

- Plan Step 9.1 chose the BIR-owned allocation-aware D5 copy-realization route.
- Initial D5 now creates explicit non-spillable `CopyScratch` reservation
  identities before E1; E1/E2 make those identities part of normal liveness
  and assignment rather than allowing a late MIR temporary.
- A subordinate D5 `CopyResolutionTransaction` consumes the exact stable
  post-E3 candidate, resolves acyclic, overlapping, and cyclic
  `ParallelCopy` groups to directly realizable `EdgeCopy` sequences, and fails
  atomically before E4 if its keyed plan or scratch reservation set is invalid.

## Suggested Next

- Execute Plan Step 9.2, "Reconcile allocation and strict downstream
  realization."

## Watchouts

- Step 9.2 must align liveness, interference, regalloc, spill/reload, allocated
  publication, and MIR consumption with the chosen pre-E1 scratch reservation
  and post-E3 resolution adjacency; it owns those detailed local contracts.
- Copy scratch is a bounded, non-spillable allocation reservation set. A
  candidate that cannot assign all simultaneously needed non-aliasing scratch
  homes fails allocation; neither the resolver nor MIR may repair it.
- The copy-resolution output creates a new exact revision. Step 10 still owns
  the single general revision-projection mechanism for constraint products;
  stable-ID preservation is not freshness proof.
- Step 14 and implementation remain forbidden pending completion of the repair
  route and a new blocker-free independent Step 13 review.

## Proof

- Passed: `git diff --check && rg -n 'D5|ParallelCopy|EdgeCopy|cycle|overlap|scratch|allocat|E3|E4|transaction|failure|MIR|directly realizable|intermediate' src/backend/bir/passes/out_of_ssa/README.md src/backend/bir/pseudo/README.md src/backend/bir/verify/README.md src/backend/bir/README.md && ! rg -n 'ParallelCopy.*(one|single).*machine instruction|MIR.*(resolve|repair|schedule).*ParallelCopy' src/backend/bir/passes/out_of_ssa/README.md src/backend/bir/pseudo/README.md src/backend/bir/verify/README.md src/backend/bir/README.md`.
- The supervisor-selected documentation proof was sufficient; this packet did
  not create or modify `test_after.log` because regression logs were explicitly
  outside packet ownership.
