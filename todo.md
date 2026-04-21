# Execution State

Status: Active
Source Idea Path: ideas/open/67_backend_trace_and_error_contract_for_x86_handoff.md
Source Plan Path: plan.md
Current Step ID: 3.2
Current Step Title: Surface A Stable Final-Rejection Contract For That Family
Plan Review Counter: 0 / 6
# Current Packet

## Just Finished

Step 3.2 now upgrades the focused `00204.c` single-block i64 immediate
return-helper family from a plain final rejection to a stable structured
contract: the route-debug renderer now emits `prepared i64 immediate
return-helper facts: opcode=..., param operand side=..., immediate operand
side=..., immediate source=...` in both summary and trace, the focused honest
`sublp0` `--dump-mir` / `--trace-mir` proof now reports
`opcode=subtract, param operand side=lhs, immediate operand side=rhs,
immediate source=direct i64`, and the reduced route-debug fixtures lock exact
facts for both the direct-immediate and sign-extended-immediate helper shapes.
`tests/backend/CMakeLists.txt` now also requires the stable fact-label surface
for the grouped `00204.c` immediate and sign-extended-immediate CLI lanes
without freezing the live opcode/source mix across all covered functions.

## Suggested Next

If idea 67 still needs another packet, target the remaining `00204.c`
single-block i64 arithmetic-right-shift return-helper rejection family, which
still exposes a plain final rejection plus `next inspect` but no structured
`final facts` contract.

## Watchouts

- The new i64 immediate facts are shape descriptors, not dynamic counts. Keep
  the stable key set (`opcode`, `param operand side`, `immediate operand
  side`, `immediate source`) even if more helper shapes are added later.
- The grouped `00204.c` CLI tests intentionally require only the fact labels
  for this family, not exact opcodes or source variants, because the covered
  functions mix direct i64 immediates with sign-extended i32 immediates.
- Focused `sublp0` proves the direct-immediate subtract case honestly; the
  sign-extended helper shape remains covered by the reduced route-debug
  fixture and the grouped CLI lane rather than a second focused manual proof.

## Proof

Ran the delegated proof command and it passed:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_route_debug|backend_cli_dump_mir_00204_i64_immediate_rejection|backend_cli_trace_mir_00204_i64_immediate_rejection|backend_cli_dump_mir_00204_i64_sign_extended_immediate_rejection|backend_cli_trace_mir_00204_i64_sign_extended_immediate_rejection)$' && build/c4cll --target x86_64-unknown-linux-gnu --dump-mir --mir-focus-function sublp0 tests/c/external/c-testsuite/src/00204.c && build/c4cll --target x86_64-unknown-linux-gnu --trace-mir --mir-focus-function sublp0 tests/c/external/c-testsuite/src/00204.c) > test_after.log 2>&1`
The resulting focused contract is:
`- final facts: prepared i64 immediate return-helper facts: opcode=subtract, param operand side=lhs, immediate operand side=rhs, immediate source=direct i64`
and the matching trace now carries the same structured final-facts line for
focused `sublp0`. Proof log path: `test_after.log`.
