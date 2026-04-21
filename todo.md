# Execution State

Status: Active
Source Idea Path: ideas/open/67_backend_trace_and_error_contract_for_x86_handoff.md
Source Plan Path: plan.md
Current Step ID: 3.4
Current Step Title: Decide Whether Idea 67 Still Needs Another Packet
Plan Review Counter: 0 / 6
# Current Packet

## Just Finished

Step 3.4 lifecycle review found no currently known meaningful `00204.c` x86
rejection family that still falls back to a plain final rejection, so idea 67
does not need another new observability-family packet. Close review is
rejected for now because the backend-scope close gate
`ctest --test-dir build -j --output-on-failure -R '^backend_'` is red on the
current tree: `backend_cli_trace_mir_00204_stdarg_rejection` still expects the
older stdarg final-detail snippet even though the current trace now matches
`local-slot-guard-chain`, and `backend_prepare_liveness` also fails in the
same guard run.

## Suggested Next

Keep idea 67 active only for one bounded proof-repair packet: refresh or
retire the stale `backend_cli_trace_mir_00204_stdarg_rejection` expectation so
the source idea's owned `00204.c` CLI contract is green again, then rerun Step
3.4 close review once the broader backend bucket is healthy. Do not invent
another rejection-family packet unless a fresh supported-CLI scan shows a
meaningful x86 family regressing back to the generic final-rejection surface.

## Watchouts

- Do not reopen idea 67 for backend capability growth. The remaining owned
  work is proof stability for the supported CLI contract, not new handoff
  matcher support.
- If the stdarg trace drift came from non-observability work elsewhere, keep
  the fix limited to honest expectation/contract alignment or route a separate
  initiative instead of stretching idea 67.
- Closure remains rejected until a matching backend close gate passes; the
  current broad backend guard also reports `backend_prepare_liveness` failing,
  so do not claim closure readiness from the focused `00204.c` scan alone.

## Proof

Close-time regression guard was run at backend scope on the clean tree with:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
Closure is rejected because the guard is red with:
`backend_cli_trace_mir_00204_stdarg_rejection`
and
`backend_prepare_liveness`
failing. Focused reruns confirmed the stdarg trace failure is a stale owned
`00204.c` expectation looking for `this helper still carries same-module call
wrappers` while the current trace now ends that helper path at
`matched local-slot-guard-chain`. Canonical logs: `test_before.log`
contains the failed backend close-gate run; no `test_after.log` comparison was
possible because the before run already failed.
