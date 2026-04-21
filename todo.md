# Execution State

Status: Active
Source Idea Path: ideas/open/67_backend_trace_and_error_contract_for_x86_handoff.md
Source Plan Path: plan.md
Current Step ID: 2.4
Current Step Title: Prove The Debug Ladder Is Coherent
Plan Review Counter: 2 / 6
# Current Packet

## Just Finished

Step 2.4 now closes the narrower `00204.c` observability gap that remained
after the full ladder continuity packet.
Focused `--dump-mir` summaries now print the matched focused BIR and prepared
block labels instead of only block counts, so the `stdarg` `entry` block can
be read directly at the MIR summary surface and correlated with the prepared
dump and focused trace without guessing from counts alone.
`tests/backend/CMakeLists.txt` tightens the `00204.c` prepared and focused-MIR
CLI coverage around that `entry` label, and
`tests/backend/backend_x86_route_debug_test.cpp` locks the summary/trace block
label contract on a smaller prepared fixture.

## Suggested Next

Decide whether Step 2.4 is now exhausted enough to advance to Step 3, or
whether any remaining `00204.c` observability gap still requires a separate
packet that is not just more expectation duplication.

## Watchouts

- The prepared-side `entry` proof still uses stable `access block=entry`
  coverage rather than allocator-sensitive frame-slot numbering.
- Summary-mode block labels should stay limited to requested focused blocks; do
  not turn the concise surface into an unfiltered per-block dump.
- If the route needs prepared-BIR-native focus controls later, treat that as a
  separate observability feature instead of stretching this packet.

## Proof

Ran the delegated proof command exactly as assigned:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_route_debug|backend_cli_(dump_prepared_bir_00204_.*|dump_mir_focus_block_.*_00204|trace_mir_focus_block_.*_00204))$' && build/c4cll --target x86_64-unknown-linux-gnu --dump-prepared-bir tests/c/external/c-testsuite/src/00204.c && build/c4cll --target x86_64-unknown-linux-gnu --dump-mir --mir-focus-function stdarg --mir-focus-block entry tests/c/external/c-testsuite/src/00204.c && build/c4cll --target x86_64-unknown-linux-gnu --trace-mir --mir-focus-function stdarg --mir-focus-block entry tests/c/external/c-testsuite/src/00204.c) > test_after.log 2>&1`
and it passed. The subset now proves the `stdarg` `entry` label survives across
the prepared dump, focused MIR summary, and focused MIR trace on the real
`00204.c` input. Proof log path: `test_after.log`.
