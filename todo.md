# Execution State

Status: Active
Source Idea Path: ideas/open/67_backend_trace_and_error_contract_for_x86_handoff.md
Source Plan Path: plan.md
Current Step ID: 3.2
Current Step Title: Surface A Stable Final-Rejection Contract For That Family
Plan Review Counter: 2 / 6
# Current Packet

## Just Finished

Step 3.2 now upgrades the remaining compare-driven `00204.c` `match` family
from a plain final rejection plus detail string to a stable structured
contract: the route-debug renderer now emits
`prepared compare-driven-entry facts: params=..., non-variadic i32 params=...,
non-i32 or varargs params=..., function variadic=...` in both summary and
trace, the reduced multi-parameter compare-driven fixture locks the exact
`params=2, non-variadic i32 params=1, non-i32 or varargs params=1, function
variadic=no` surface, and the focused honest `00204.c` `match`
`--dump-mir` / `--trace-mir` CLI tests now lock the real large-case contract
with `params=2, non-variadic i32 params=0, non-i32 or varargs params=2,
function variadic=no` alongside the existing next-inspect guidance.

## Suggested Next

The broad `00204.c` scan no longer shows a currently known meaningful x86
rejection family that still falls back to a plain final rejection without
structured facts. Route the next action to Step 3.4 lifecycle review so the
plan can decide whether idea 67 is ready for closure/completion review instead
of inventing another observability packet.

## Watchouts

- Keep the compare-driven facts shape-oriented and signature-oriented. The
  stable key set is `params`, `non-variadic i32 params`, `non-i32 or varargs
  params`, and `function variadic`; avoid backend-internal labels or block
  names that would make the contract brittle.
- The reduced route-debug fixture and the focused `00204.c` `match` proof
  intentionally disagree on the exact counts because they cover different
  signatures. Preserve the shared key set rather than forcing the real case to
  mirror the reduced fixture.
- The focused `match` CLI proof is enough to lock the real large-case contract;
  do not widen this packet into compare-driven capability work under idea 67.

## Proof

Ran the delegated proof command and it passed:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_x86_route_debug$' && build/c4cll --target x86_64-unknown-linux-gnu --dump-mir --mir-focus-function match tests/c/external/c-testsuite/src/00204.c && build/c4cll --target x86_64-unknown-linux-gnu --trace-mir --mir-focus-function match tests/c/external/c-testsuite/src/00204.c) > test_after.log 2>&1`
The resulting focused contract is:
`- final facts: prepared compare-driven-entry facts: params=2, non-variadic i32 params=0, non-i32 or varargs params=2, function variadic=no`
and the matching trace now carries the same structured final-facts line for
focused `match`. Supplemental CLI tests also passed:
`ctest --test-dir build -j --output-on-failure -R '^(backend_cli_dump_mir_00204_match_rejection|backend_cli_trace_mir_00204_match_rejection)$'`
Proof log path: `test_after.log`.
