# Execution State

Status: Active
Source Idea Path: ideas/open/67_backend_trace_and_error_contract_for_x86_handoff.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Continue Until Backend Handoff Debugging Stops Requiring Local Instrumentation
Plan Review Counter: 1 / 6
# Current Packet

## Just Finished

Step 3 now extends the focused large-case dump contract from blocks down to one
value: `--mir-focus-value` exists for `--dump-bir` and `--dump-prepared-bir`,
normalizes plain names like `t1` to the backend `%t1` spelling, and refuses to
run without `--mir-focus-function`. The semantic and prepared dump paths now
keep the focused `stdarg` function but trim its output to value-owned lines
only, while the focus header reports explicit semantic/prepared matched-count
totals so a large `00204.c` investigation can isolate one value without local
instrumentation. `tests/backend/CMakeLists.txt` adds matching and missing
`00204.c` CLI coverage for semantic and prepared value focus so the new seam is
proved on the real `stdarg` handoff lane.

## Suggested Next

If idea 67 still needs another packet, keep it on the next real observability
gap near backend handoff, such as whether MIR summary/trace needs an analogous
value seam or whether the remaining large-case rejection text still forces
source edits for local inspection. Do not spend more packets polishing the dump
header without a concrete inspection failure.

## Watchouts

- `--mir-focus-value` is intentionally dump-only for now and still requires
  `--mir-focus-function`; do not silently broaden it into module-level or MIR
  summary/trace matching without a separate packet.
- The prepared value filter keeps function-level notes and frame metadata even
  when no value-owned prepared records match; preserve that route context unless
  a later packet proves it blocks inspection.
- The current matching contract is value-owned line filtering, not SSA
  dependency slicing; do not overfit follow-up work into ad hoc producer or
  consumer expansion around one testcase name.

## Proof

Ran the delegated acceptance subset and spot checks:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_cli_(dump_bir_(focus_function_filters_00204|focus_block_(entry|missing)_00204|focus_value_(t1|missing)_00204)|dump_prepared_bir_(focus_function_filters_00204|focus_block_(entry|missing)_00204|focus_value_(t1|missing)_00204)))$' && build/c4cll --target x86_64-unknown-linux-gnu --dump-bir --mir-focus-function stdarg --mir-focus-value t1 tests/c/external/c-testsuite/src/00204.c && build/c4cll --target x86_64-unknown-linux-gnu --dump-prepared-bir --mir-focus-function stdarg --mir-focus-value t1 tests/c/external/c-testsuite/src/00204.c) > test_after.log 2>&1`
and it passed. The proof locks matching and missing value focus on semantic and
prepared dumps and spot-checks the real `00204.c` `stdarg` `t1` value through
both dump stages. Proof log path: `test_after.log`.
