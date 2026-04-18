Status: Active
Source Idea: ideas/open/54_x86_backend_c_testsuite_capability_families.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed `plan.md` Step 2 ("Lock The Family Boundary In Backend Tests") by
updating backend-owned proofs so the first scalar-control-flow lane is
described as the bounded compare-against-zero branch route, while the current
failing cluster remains represented by an unsupported switch/label-shaped
scalar-control-flow notes fixture anchored to `00051`, `00158`, `00193`,
`00213`, and `00215`. Idea `56` remains parallel and out of scope for this
packet.

## Suggested Next

Execute `plan.md` Step 3 for the chosen scalar-control-flow lane: implement one
honest lowering repair that widens support beyond the current compare-against-
zero branch lane toward the switch/label-heavy cluster without touching idea
`56` or weakening backend expectations.

## Watchouts

- Do not reopen idea `55`; the memory ownership split is closed.
- Treat idea `56` as a separate initiative; the current Step 1 baseline did
  not show renderer de-headerization as the immediate blocker.
- Do not weaken `x86_backend` expectations or add testcase-shaped shortcuts.
- Keep the first implementation packet scoped to one scalar-control-flow lane,
  not the whole control-flow family or the whole c-testsuite backlog.
- The backend tests now state the boundary as: compare-against-zero branch
  support is in-lane; switch/label-heavy control flow is still out of lane.

## Proof

Executor packet proof:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_(lir_to_bir_notes|x86_handoff_boundary_test)$" |& tee test_after.log'`

Result: the delegated command built successfully and preserved `test_after.log`,
but its regex only matched `backend_lir_to_bir_notes`. Supervisor acceptance
also ran `ctest --test-dir build -j --output-on-failure -R '^backend_(lir_to_bir_notes|x86_handoff_boundary)$'`,
which passed both backend boundary tests.
