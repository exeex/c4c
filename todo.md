# Current Packet

Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 2.2
Current Step Title: Close core architecture choices and repair its BIR disposition

## Just Finished

- Plan Step 2.2 closed the A2 core contract for the versioned intrinsic
  registry, paired asm-goto topology/results, bounded debug/origin schema,
  fail-closed unwind-edge policy, explicit-phi SSA, and exact `EdgeKey` schema.
- Reclassified allocation state so Raw/Canonical core storage excludes it while
  E1 owns liveness/interference, E2 owns abstract homes, and E3 owns explicit
  Pseudo BIR spill/reload state; remaining gaps are implementation or typed
  source-carrier gaps rather than unresolved architecture.

## Suggested Next

- Execute Plan Step 3.1, "Propagate the A-F registry through infrastructure,"
  reviewing pipeline, passes, then analysis contracts in that order.

## Watchouts

- Preserve the closed A2 semantic choices while removing obsolete stage IDs;
  Step 3.1 changes registry labels and edges, not representation decisions.
- Step 14 and implementation remain forbidden pending completion of the repair
  route and a new blocker-free independent Step 13 review.

## Proof

- Passed: `git diff --check && ! rg -n 'interval.*outside BIR|home.*outside
  BIR|spill/reload.*outside BIR|later plans/MIR|Open.*(intrinsic|asm-goto|debug|unwind|SSA|EdgeKey)'
  src/backend/bir/core/README.md && rg -n
  'Raw/Canonical|E1|E2|E3|intrinsic|asm-goto|debug|unwind|EdgeKey|source gap|implementation'
  src/backend/bir/core/README.md`.
- The supervisor-selected documentation proof was sufficient; this packet did
  not create or modify `test_after.log` because regression logs were explicitly
  outside packet ownership.
