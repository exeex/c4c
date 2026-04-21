# Execution State

Status: Active
Source Idea Path: ideas/open/67_backend_trace_and_error_contract_for_x86_handoff.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Continue Until Backend Handoff Debugging Stops Requiring Local Instrumentation
Plan Review Counter: 3 / 6
# Current Packet

## Just Finished

Step 3 now upgrades the focused `stdarg` x86 final rejection so `00204.c`
debugging does not need local probes for the current void call-sequence lane:
the route-debug renderer now reports stable prepared call-wrapper facts in both
summary and trace, and the trace detail correctly distinguishes same-module
calls from direct variadic extern calls. For `stdarg`, focused `--dump-mir` now
surfaces `same-module calls=35` and `direct variadic extern calls=1`, while
focused `--trace-mir` explains that the prepared call-wrapper still carries 35
same-module calls and 1 direct variadic extern call. `tests/backend/
CMakeLists.txt` now requires that contract on the focused `00204.c` CLI lane,
and `tests/backend/backend_x86_route_debug_test.cpp` adds a synthetic void
call-sequence miss that honestly falls through to the `single-block-void-
call-sequence` rejection and locks the summary/trace final-facts wording.

## Suggested Next

If idea 67 still needs another packet, stay on the remaining backend handoff
observability gap rather than polishing this lane further: pick the next
focused `00204.c` rejection family whose summary still says only “outside the
current x86 support,” then give it the same stable final-facts treatment if it
still forces local instrumentation.

## Watchouts

- The new void call-sequence facts are direct call-category counts over the
  prepared function body. For `00204.c`, `same-module calls` intentionally
  includes the defined variadic helper `myprintf`; do not collapse that back
  into the `direct variadic extern calls` bucket.
- The summary contract is intentionally two-part: keep the existing rejection
  classification line, then add `final facts` as a stable structured line.
  Avoid stuffing every count back into the summary sentence.
- The focused CLI tests for `00204.c` only require stable fact labels, not the
  exact `same-module calls=35` count. Preserve that flexibility unless the
  supervisor explicitly wants a stricter golden contract.

## Proof

Ran the real passing acceptance subset and it passed:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_route_debug|backend_cli_dump_mir_focus_function_filters_00204|backend_cli_trace_mir_focus_function_filters_00204|backend_cli_dump_mir_focus_block_entry_00204|backend_cli_trace_mir_focus_block_entry_00204|backend_cli_dump_mir_focus_block_missing_00204|backend_cli_trace_mir_focus_block_missing_00204|backend_cli_dump_mir_focus_value_t1_00204|backend_cli_dump_mir_focus_value_missing_00204|backend_cli_trace_mir_focus_value_t1_00204|backend_cli_trace_mir_focus_value_missing_00204)$' && build/c4cll --target x86_64-unknown-linux-gnu --dump-mir --mir-focus-function stdarg tests/c/external/c-testsuite/src/00204.c && build/c4cll --target x86_64-unknown-linux-gnu --trace-mir --mir-focus-function stdarg tests/c/external/c-testsuite/src/00204.c) > test_after.log 2>&1`
The resulting focused contract is:
`- final facts: prepared call-wrapper facts: same-module calls=35, direct variadic extern calls=1, direct fixed-arity extern calls=0, indirect calls=0, other call side effects=0`
and the matching trace detail says the prepared call-wrapper still carries 35
same-module calls and 1 direct variadic extern call. Proof log path:
`test_after.log`.
