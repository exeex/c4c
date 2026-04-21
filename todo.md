# Execution State

Status: Active
Source Idea Path: ideas/open/67_backend_trace_and_error_contract_for_x86_handoff.md
Source Plan Path: plan.md
Current Step ID: 3.1
Current Step Title: Isolate The Next Remaining Meaningful Rejection Family
Plan Review Counter: 0 / 6
# Current Packet

## Just Finished

Step 3 now upgrades the focused `myprintf` x86 final rejection so `00204.c`
debugging does not stop at a plain unsupported-shape rejection for the bounded
same-module variadic helper lane: the route-debug renderer now emits stable
structured variadic-helper facts in both summary and trace, and the focused
`--dump-mir` / `--trace-mir` lane now surfaces `prepared variadic-helper
facts: explicit variadic runtime calls=1, same-module helper calls=14, direct
extern calls=16, indirect calls=0, other call side effects=0`. The nearest
synthetic route-debug test now locks the exact summary/trace final-facts
contract for a bounded non-entry variadic helper miss, and
`tests/backend/CMakeLists.txt` requires the stable fact labels on the focused
`00204.c` CLI lane without freezing the live `myprintf` counts.

## Suggested Next

If idea 67 still needs another packet, stay on the remaining focused
`00204.c` backend handoff observability gaps rather than polishing this lane
further: pick the next focused rejection family whose summary still stops at
“outside the current x86 support,” then add the same style of stable
structured final facts plus a synthetic route-debug contract if local
instrumentation is still required.

## Watchouts

- The new variadic-helper facts count BIR call-family structure inside the
  focused helper, not source-level declarations. In focused `00204.c`,
  `same-module helper calls=14` and `direct extern calls=16` reflect the
  prepared `myprintf` body after lowering, alongside the explicit `va_*`
  runtime count.
- The summary contract is intentionally two-part: keep the existing rejection
  classification line, then add `final facts` as a stable structured line.
  Avoid pushing every count back into the rejection sentence.
- The focused CLI tests for `00204.c` only require stable fact labels, not the
  exact `1` / `14` / `16` live counts from `myprintf`. Preserve that
  flexibility unless the supervisor explicitly wants a stricter golden
  contract.

## Proof

Ran the delegated proof command and it passed:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_route_debug|backend_cli_dump_mir_00204_myprintf_rejection|backend_cli_trace_mir_00204_myprintf_rejection)$' && build/c4cll --target x86_64-unknown-linux-gnu --dump-mir --mir-focus-function myprintf tests/c/external/c-testsuite/src/00204.c && build/c4cll --target x86_64-unknown-linux-gnu --trace-mir --mir-focus-function myprintf tests/c/external/c-testsuite/src/00204.c) > test_after.log 2>&1`
The resulting focused contract is:
`- final facts: prepared variadic-helper facts: explicit variadic runtime calls=1, same-module helper calls=14, direct extern calls=16, indirect calls=0, other call side effects=0`
and the matching trace now carries the same structured final-facts line for
focused `myprintf`. Proof log path: `test_after.log`.
