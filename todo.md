# Execution State

Status: Active
Source Idea Path: ideas/open/67_backend_trace_and_error_contract_for_x86_handoff.md
Source Plan Path: plan.md
Current Step ID: 2.3
Current Step Title: Add Focus Controls For Large Cases
Plan Review Counter: 1 / 6
# Current Packet

## Just Finished

Step 2.3 started with stable function-level focus controls for the x86 MIR
debug ladder.
`src/apps/c4cll.cpp` now accepts `--mir-focus-function <name>` for
`--dump-mir` and `--trace-mir`, `src/backend/backend.{hpp,cpp}` threads that
focus selection into backend dump options, and
`src/backend/mir/x86/codegen/route_debug.cpp` keeps the module-level handoff
summary while filtering per-function route output to the named function only.
The focused report emits an explicit `focus function:` header plus a
`focused functions matched:` footer so large cases stay readable without
guesswork about whether filtering applied. Acceptance proof adds focused
`00204.c` CLI coverage in `tests/backend/CMakeLists.txt` and extends the
shared backend dump harness to pass list-shaped extra CLI arguments.

## Suggested Next

Continue Step 2.3 by adding a second narrowing seam inside the selected
function, preferably block-label focus for `--trace-mir`, so `00204.c`-scale
inspection can shrink noisy helper traces further without hiding the final
rejection or module-level context.

## Watchouts

- The new focus control is diagnostic-only: it must not change lane ordering,
  final-rejection selection, or backend support decisions.
- Module-level handoff context still prints even when a function focus is set;
  keep that intact for future Step 2.3 packets so filtering does not hide the
  first decisive module-level rejection.
- The shared backend dump harness now accepts `EXTRA_ARGS`; future CLI tests
  should keep quoting list-valued `-D...` arguments in `tests/backend/CMakeLists.txt`
  so CTest does not split them before `cmake -P` sees the list.
- Keep Step 2.3 proof anchored on honest `00204.c` CLI output plus the nearest
  backend CLI/route-debug subset; do not replace large-case proof with only
  reduced fixtures.

## Proof

Ran the delegated proof command exactly as assigned:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_route_debug|backend_cli_(dump_mir_is_nonfatal_trace_shell|trace_mir_reports_lane_detail|dump_mir_00204_stdarg_rejection|trace_mir_00204_stdarg_rejection|dump_mir_00204_arg_rejection|trace_mir_00204_arg_rejection|dump_mir_00204_opi_rejection|trace_mir_00204_opi_rejection|dump_mir_focus_function_filters_00204|trace_mir_focus_function_filters_00204))$' && build/c4cll --target x86_64-unknown-linux-gnu --dump-mir --mir-focus-function stdarg tests/c/external/c-testsuite/src/00204.c && build/c4cll --target x86_64-unknown-linux-gnu --trace-mir --mir-focus-function stdarg tests/c/external/c-testsuite/src/00204.c) > test_after.log 2>&1`
and it passed. Fresh focused live output for `00204.c` now keeps the
module-level rejection context while restricting the per-function MIR report to
`stdarg`, and it reports `focused functions matched: 1` in both summary and
trace mode.
