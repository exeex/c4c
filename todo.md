# Execution State

Status: Active
Source Idea Path: ideas/open/67_backend_trace_and_error_contract_for_x86_handoff.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Continue Until Backend Handoff Debugging Stops Requiring Local Instrumentation
Plan Review Counter: 4 / 6
# Current Packet

## Just Finished

Step 3 now upgrades the focused `opi` x86 final rejection so `00204.c`
debugging does not need local instrumentation for the same-module scalar
call-wrapper family: the route-debug renderer now reports stable prepared
helper-family facts in both summary and trace, and the scalar-wrapper trace
detail is derived from those structured counts instead of a fixed prose list.
For focused `--dump-mir` and `--trace-mir`, `opi` now surfaces
`prepared helper-family facts: local-slot reloads=0, scalar same-module helper
calls=36, width-adjusting casts=36, same-module sink wrappers=36`.
`tests/backend/CMakeLists.txt` now requires those stable fact labels on the
focused `00204.c` CLI lane, and
`tests/backend/backend_x86_route_debug_test.cpp` locks the exact summary/trace
final-facts wording on the synthetic scalar-wrapper miss.

## Suggested Next

If idea 67 still needs another packet, stay on the remaining backend handoff
observability gap rather than polishing this lane further: pick the next
focused `00204.c` rejection family whose summary still says only “outside the
current x86 support,” then give it the same stable final-facts treatment if it
still forces local instrumentation.

## Watchouts

- The new scalar helper-family facts are counted from the prepared wrapper
  chain, not from source-level syntax. In focused `00204.c`, prepared lowering
  folds the scalar family to `local-slot reloads=0` even though the synthetic
  unit test still exercises the reload bucket explicitly.
- The summary contract is intentionally two-part: keep the existing rejection
  classification line, then add `final facts` as a stable structured line.
  Avoid pushing every count back into the rejection sentence.
- The focused CLI tests for `00204.c` only require stable fact labels, not the
  exact `36` counts. Preserve that flexibility unless the supervisor
  explicitly wants a stricter golden contract.

## Proof

Ran the delegated proof command and it passed:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_route_debug|backend_cli_dump_mir_00204_opi_rejection|backend_cli_trace_mir_00204_opi_rejection)$' && build/c4cll --target x86_64-unknown-linux-gnu --dump-mir --mir-focus-function opi tests/c/external/c-testsuite/src/00204.c && build/c4cll --target x86_64-unknown-linux-gnu --trace-mir --mir-focus-function opi tests/c/external/c-testsuite/src/00204.c) > test_after.log 2>&1`
The resulting focused contract is:
`- final facts: prepared helper-family facts: local-slot reloads=0, scalar same-module helper calls=36, width-adjusting casts=36, same-module sink wrappers=36`
and the matching trace detail says the prepared wrapper family still carries 36
scalar same-module helper calls, 36 width-adjusting casts, and 36 same-module
sink wrappers. Proof log path: `test_after.log`.
