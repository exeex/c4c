# Execution State

Status: Active
Source Idea Path: ideas/open/67_backend_trace_and_error_contract_for_x86_handoff.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Continue Until Backend Handoff Debugging Stops Requiring Local Instrumentation
Plan Review Counter: 2 / 6
# Current Packet

## Just Finished

Step 3 now extends the focused value seam into MIR summary and trace: `c4cll`
accepts `--mir-focus-value` with `--dump-mir` and `--trace-mir`, mirrors that
focus into the x86 route-debug renderer, and preserves the decisive `stdarg`
rejection guidance on `00204.c` while reporting focused prepared-value and
move-bundle match counts for the requested value. `tests/backend/CMakeLists.txt`
now adds matching and missing `00204.c` CLI coverage for MIR summary/trace
value focus, and `tests/backend/backend_x86_route_debug_test.cpp` locks the
route-debug renderer’s focused-value headers and missing-value note.

## Suggested Next

If idea 67 still needs another packet, stay on the remaining backend handoff
observability gap rather than polishing this seam further: either expose the
same narrow focus deeper in the prepared handoff detail when a rejection still
needs local probes, or move to the next missing rejection contract that still
blocks `00204.c`-scale debugging.

## Watchouts

- The MIR value seam is currently a CLI-to-route-debug bridge via
  `C4C_MIR_FOCUS_VALUE`; if a later packet needs non-CLI callers to use the
  same focus directly, that should become an explicit backend API-plumbing
  packet instead of hidden widening here.
- MIR summary/trace value focus is intentionally context-preserving: it reports
  focused prepared-value metadata but does not suppress the final function-level
  rejection text. Do not trade away that rejection context for narrower output
  without a concrete debugging failure.
- The new MIR counts are prepared-handoff counts, not SSA dependency slicing;
  avoid growing this into testcase-shaped producer/consumer expansion around
  one named value.

## Proof

Ran the delegated acceptance subset and spot checks:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_cli_(dump_mir|trace_mir)_focus_(function_filters_00204|block_(entry|missing)_00204|value_(t1|missing)_00204))$' && build/c4cll --target x86_64-unknown-linux-gnu --dump-mir --mir-focus-function stdarg --mir-focus-value t1 tests/c/external/c-testsuite/src/00204.c && build/c4cll --target x86_64-unknown-linux-gnu --trace-mir --mir-focus-function stdarg --mir-focus-value t1 tests/c/external/c-testsuite/src/00204.c) > test_after.log 2>&1`
and it passed. I also ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_x86_route_debug$'`
to verify the owned route-debug unit coverage after adding focused-value
expectations. Proof log path: `test_after.log`.
