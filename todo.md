# Execution State

Status: Active
Source Idea Path: ideas/open/67_backend_trace_and_error_contract_for_x86_handoff.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Continue Until Backend Handoff Debugging Stops Requiring Local Instrumentation
Plan Review Counter: 5 / 6
# Current Packet

## Just Finished

Step 3 now upgrades the focused `arg` x86 final rejection so `00204.c`
debugging does not stop at a plain unsupported-shape rejection for the
single-block aggregate-forwarding wrapper family: the route-debug renderer now
reports stable structured aggregate-wrapper facts in both summary and trace,
and the aggregate-wrapper trace detail is derived from those counts instead of
the old generic prose. For focused `--dump-mir` and `--trace-mir`, `arg` now
surfaces `prepared aggregate-wrapper facts: direct extern calls=1,
same-module aggregate call wrappers=33, forwarded aggregate arguments=50`.
`tests/backend/CMakeLists.txt` now requires those stable fact labels on the
focused `00204.c` CLI lane, and
`tests/backend/backend_x86_route_debug_test.cpp` locks the exact summary/trace
final-facts wording on the synthetic aggregate-forwarding wrapper miss.

## Suggested Next

If idea 67 still needs another packet, stay on the remaining backend handoff
observability gap rather than polishing this lane further: pick the next
focused `00204.c` rejection family whose summary still says only “outside the
current x86 support,” then give it the same stable final-facts treatment if it
still forces local instrumentation.

## Watchouts

- The new aggregate-wrapper facts count wrapper-family structure from the
  prepared same-module calls, not from source declarations. In focused
  `00204.c`, `forwarded aggregate arguments=50` is larger than the
  `same-module aggregate call wrappers=33` count because multi-aggregate helper
  calls like `fa1`/`fa2`/`fa4` each contribute multiple forwarded aggregate
  parameters.
- The summary contract is intentionally two-part: keep the existing rejection
  classification line, then add `final facts` as a stable structured line.
  Avoid pushing every count back into the rejection sentence.
- The focused CLI tests for `00204.c` only require stable fact labels, not the
  exact `33` / `50` counts. Preserve that flexibility unless the supervisor
  explicitly wants a stricter golden contract.

## Proof

Ran the delegated proof command and it passed:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_route_debug|backend_cli_dump_mir_00204_arg_rejection|backend_cli_trace_mir_00204_arg_rejection)$' && build/c4cll --target x86_64-unknown-linux-gnu --dump-mir --mir-focus-function arg tests/c/external/c-testsuite/src/00204.c && build/c4cll --target x86_64-unknown-linux-gnu --trace-mir --mir-focus-function arg tests/c/external/c-testsuite/src/00204.c) > test_after.log 2>&1`
The resulting focused contract is:
`- final facts: prepared aggregate-wrapper facts: direct extern calls=1, same-module aggregate call wrappers=33, forwarded aggregate arguments=50`
and the matching trace detail says the prepared wrapper family still carries 1
direct extern call, 33 same-module aggregate call wrappers, and 50 forwarded
aggregate arguments. Proof log path: `test_after.log`.
