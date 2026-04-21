# Execution State

Status: Active
Source Idea Path: ideas/open/67_backend_trace_and_error_contract_for_x86_handoff.md
Source Plan Path: plan.md
Current Step ID: 2.2.3
Current Step Title: Eliminate Remaining Generic Per-Function Misses In The Motivating Case
Plan Review Counter: 6 / 6
# Current Packet

## Just Finished

Step 2.2.3 progressed for the remaining ordinary per-function misses in
`tests/c/external/c-testsuite/src/00204.c`.
`src/backend/mir/x86/codegen/route_debug.cpp` now lets the existing
single-block void call-sequence detector classify local-slot-backed wrappers,
which restores the intended lane-specific final rejection for `stdarg`, and it
now treats sign- or zero-extended i32 constant builders as immediate-like for
the existing single-block i64 immediate return-helper detector. That converts
the `addlm123`, `andlm1`, and `eorlm1` helper family from generic per-function
misses into the established immediate-return rejection class without widening
backend support or keying off testcase names. Acceptance proof now includes
updated reduced route-debug fixture coverage in
`tests/backend/backend_x86_route_debug_test.cpp` for the extended-immediate
shape and new honest CLI coverage in `tests/backend/CMakeLists.txt` for the
`00204.c` sign-extended i64 helper family, while the existing `stdarg` CLI
tests now pass again against the live trace.

## Suggested Next

Step 2.2.3 now appears exhausted on the motivating `00204.c` case: fresh
`--dump-mir` and `--trace-mir` output no longer show any remaining ordinary
per-function miss in the live trace. Advance to Step 2.3 and focus on stable
trace filtering or other large-case focus controls that preserve the final
meaningful rejection while reducing search space on `00204.c`-scale inputs.

## Watchouts

- The widened void-helper detector is still diagnostic-only: it remains
  restricted to single-block `void` helpers with observable call effects and
  does not widen actual x86 helper support.
- The widened i64 immediate detector is still shape-based rather than
  testcase-based: it only treats a literal i64 immediate or an explicit
  `sub i32 0, imm` plus `sext`/`zext` builder as the same immediate-like
  contract before the final return operation.
- Keep Step 2.2 proof anchored on `tests/c/external/c-testsuite/src/00204.c`;
  reduced fixtures here remain route-debug contract coverage, not the only
  acceptance signal.
- Step 2.3 should preserve the current final-rejection contract while adding
  focus controls; it should not reopen matcher coverage or weaken the stable
  x86 rejection wording that now covers the motivating case.

## Proof

Ran the delegated proof command exactly as assigned:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_route_debug|backend_cli_(dump_mir_00204_stdarg_rejection|trace_mir_00204_stdarg_rejection|dump_mir_00204_i64_immediate_rejection|trace_mir_00204_i64_immediate_rejection|dump_mir_00204_i64_sign_extended_immediate_rejection|trace_mir_00204_i64_sign_extended_immediate_rejection))$' && build/c4cll --target x86_64-unknown-linux-gnu --dump-mir tests/c/external/c-testsuite/src/00204.c && build/c4cll --target x86_64-unknown-linux-gnu --trace-mir tests/c/external/c-testsuite/src/00204.c) > test_after.log 2>&1`
and it passed. Fresh live `--dump-mir` and `--trace-mir` output for
`00204.c` now shows lane-specific final rejections for `stdarg`,
`addlm123`, `andlm1`, and `eorlm1`, and the motivating trace no longer emits
the generic ordinary per-function miss.
