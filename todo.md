# Execution State

Status: Active
Source Idea Path: ideas/open/67_backend_trace_and_error_contract_for_x86_handoff.md
Source Plan Path: plan.md
Current Step ID: 3.4
Current Step Title: Decide Whether Idea 67 Still Needs Another Packet
Plan Review Counter: 1 / 6
# Current Packet

## Just Finished

Step 3.4 finished the bounded `00204.c` stdarg trace proof-repair packet for
idea 67 by refreshing `backend_cli_trace_mir_00204_stdarg_rejection` to the
current supported CLI contract. The trace still rejects `function stdarg`
through `single-block-void-call-sequence`, but its detail now reports counted
prepared call-wrapper facts (`35 same-module calls` and `1 direct variadic
extern call`) instead of the older generic same-module-wrapper wording.

## Suggested Next

Rerun the Step 3.4 close review only after the separate backend blocker
`backend_prepare_liveness` is repaired or otherwise cleared. Do not reopen idea
67 for new observability-family work unless a fresh supported-CLI scan shows a
real `00204.c` regression back to an opaque final rejection.

## Watchouts

- Keep idea 67 scoped to observability-contract truthfulness. This packet only
  refreshed the stale trace expectation; it did not change backend capability.
- Closure is still blocked outside this packet because the broader backend
  close gate remains red on `backend_prepare_liveness`.
- If the stdarg trace wording drifts again, prefer matching the concrete
  supported CLI contract over reintroducing vague helper-shape wording.

## Proof

Supervisor-selected proof passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_cli_dump_mir_00204_stdarg_rejection|backend_cli_trace_mir_00204_stdarg_rejection)$'`
Both owned `00204.c` stdarg CLI tests are green. Canonical executor log:
`test_after.log`. The earlier close-review baseline remains in `test_before.log`
and still records the separate non-owned blocker `backend_prepare_liveness`.
