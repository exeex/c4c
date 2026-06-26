Status: Active
Source Idea Path: ideas/open/380_rv64_object_route_short_circuit_call_argument_reload.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Residual Prior-Preservation Gap

# Current Packet

## Just Finished

Plan-owner repair rewrote the exhausted Step 4 closure/handoff state into a
new bounded runbook for the still in-scope idea 380 residual. The prior Step 4
classification remains the handoff evidence: `src/20000112-1.c` still reaches
the second `strchr` with `a0 == NULL` after the prior repair admitted one GPR
callee-saved `PriorPreservation` shape.

## Suggested Next

Execute Step 1 in `plan.md`: audit why the prior-preservation repair does not
keep the original incoming `fmt` pointer available after the first failed
`strchr` call and before the later prepared call.

## Watchouts

Do not split this into a separate runtime-crash idea on the current evidence.
The remaining crash is still the call-argument reload/prior-preservation owner
from idea 380. Avoid testcase-shaped handling for `src/20000112-1.c`,
`special_format`, or a particular `strchr` call.

## Proof

Lifecycle-only repair; no code validation or root proof logs were run. The
handoff evidence remains in
`build/agent_state/380_step4_20000112.classification.txt` and related
`build/agent_state/380_step4_20000112.*` artifacts.
