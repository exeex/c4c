Status: Active
Source Idea Path: ideas/open/90_out_of_ssa_critical_edge_and_parallel_copy_deepening.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Make Ordering, Cycle Breaking, And Carrier Use Truthful
Plan Review Counter: 2 / 6
# Current Packet

## Just Finished

Step 3 now publishes explicit out-of-SSA authority for prepared move
resolution: regalloc/value-location bundle placement no longer depends on
`move.reason` starting with `phi_`; prepared move bundles carry a first-class
authority kind sourced from the published parallel-copy bundle; and the focused
handoff/liveness proofs now assert that explicit authority instead of the
textual fallback.

## Suggested Next

Stay within Step 3 and pick the next narrow downstream consumer that still
reconstructs out-of-SSA ordering, cycle-temp use, or carrier/storage facts
instead of consuming the published prepared bundle authority directly.

## Watchouts

- The new move authority only covers the out-of-SSA parallel-copy path; keep
  later packets from reintroducing reason-string inference in downstream
  helpers or tests.
- Preserve the existing target-independent execution-site contract:
  predecessor, successor-entry, and critical-edge placement should continue to
  flow from published bundle authority, not x86-local reconstruction.
- Keep cycle-break and edge-store-slot carrier facts sourced from the
  published parallel-copy bundle rather than duplicating them in secondary
  textual shims.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
Passed. Backend build and the `^backend_` subset stayed green after publishing
explicit out-of-SSA move authority and updating the focused compare-branch,
loop-countdown, short-circuit, and liveness proofs; proof log written to
`test_after.log`.
