# Execution State

Status: Active
Source Idea Path: ideas/open/67_backend_trace_and_error_contract_for_x86_handoff.md
Source Plan Path: plan.md
Current Step ID: 3.2
Current Step Title: Surface A Stable Final-Rejection Contract For That Family
Plan Review Counter: 1 / 6
# Current Packet

## Just Finished

Step 3.2 now upgrades the remaining opaque `00204.c` single-block i64
arithmetic-right-shift return-helper family from a plain final rejection to a
stable structured contract: the route-debug renderer now emits
`prepared i64 arithmetic-right-shift return-helper facts: param operand
side=..., shift operand side=..., shift source=...` in both summary and
trace, the focused honest `asrl0` `--dump-mir` / `--trace-mir` proof now
reports `param operand side=lhs, shift operand side=rhs, shift source=direct
i64`, and the focused route-debug fixtures plus grouped `00204.c` CLI
rejection tests now lock that stable final-facts surface alongside the
existing next-inspect guidance.

## Suggested Next

No next opaque `00204.c` single-block i64 return-helper rejection family is
known from this packet. If idea 67 still needs another packet, pick the next
remaining `00204.c` rejection family outside this i64 return-helper subset
that still lacks a stable structured final-facts contract.

## Watchouts

- The new arithmetic-right-shift facts are shape descriptors, not shift-count
  snapshots. Keep the stable key set (`param operand side`, `shift operand
  side`, `shift source`) so `asrl0`, `asrl1`, and `asrl63` can share one
  contract.
- The grouped `00204.c` CLI lane now locks the exact stable facts because the
  covered arithmetic-right-shift helpers share the same operand-side/source
  shape across the family.
- Focused `asrl0` proves the zero-shift member honestly; `asrl1` and `asrl63`
  stay covered by the grouped CLI rejection lane rather than extra focused
  manual proofs.

## Proof

Ran the delegated proof command and it passed:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_route_debug|backend_cli_dump_mir_00204_asrl_rejection|backend_cli_trace_mir_00204_asrl_rejection)$' && build/c4cll --target x86_64-unknown-linux-gnu --dump-mir --mir-focus-function asrl0 tests/c/external/c-testsuite/src/00204.c && build/c4cll --target x86_64-unknown-linux-gnu --trace-mir --mir-focus-function asrl0 tests/c/external/c-testsuite/src/00204.c) > test_after.log 2>&1`
The resulting focused contract is:
`- final facts: prepared i64 arithmetic-right-shift return-helper facts: param operand side=lhs, shift operand side=rhs, shift source=direct i64`
and the matching trace now carries the same structured final-facts line for
focused `asrl0`. Proof log path: `test_after.log`.
