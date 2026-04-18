Status: Active
Source Idea: ideas/open/54_x86_backend_c_testsuite_capability_families.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed `plan.md` Step 1 ("Re-Baseline And Choose The First Family") by
capturing a fresh `x86_backend` baseline in `test_before.log`, re-clustering
the visible failures, and selecting semantic scalar-control-flow as the first
family. The initial bounded proving lane should stay inside scalar-control-flow
cases centered on switch/branch-heavy control paths such as
`00051`, `00158`, `00193`, `00213`, and `00215`. Idea `56` remains parallel,
not an immediate blocker for this first capability packet.

## Suggested Next

Execute `plan.md` Step 2 for the chosen scalar-control-flow lane: update the
backend notes and x86 handoff tests so they describe one bounded
scalar-control-flow boundary truthfully before any lowering change widens
support.

## Watchouts

- Do not reopen idea `55`; the memory ownership split is closed.
- Treat idea `56` as a separate initiative; the current Step 1 baseline did
  not show renderer de-headerization as the immediate blocker.
- Do not weaken `x86_backend` expectations or add testcase-shaped shortcuts.
- Keep the first packet scoped to one scalar-control-flow lane, not the whole
  control-flow family or the whole c-testsuite backlog.
- The current baseline is materially better than the 202-fail snapshot in the
  source idea: `32% tests passed, 150 tests failed out of 220`, with
  scalar-control-flow currently the largest named semantic bucket at 10 visible
  cases and `gep` local-memory next at 7.

## Proof

Supervisor baseline run:
`cmake --build --preset default && ctest --test-dir build -L x86_backend --output-on-failure`

Result: baseline captured in `test_before.log`; the command is expected to exit
nonzero because the active x86 backend route still has failing cases.
