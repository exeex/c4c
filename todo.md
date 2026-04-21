# Execution State

Status: Active
Source Idea Path: ideas/open/67_backend_trace_and_error_contract_for_x86_handoff.md
Source Plan Path: plan.md
Current Step ID: 2.4
Current Step Title: Prove The Debug Ladder Is Coherent
Plan Review Counter: 0 / 6
# Current Packet

## Just Finished

Step 2.3 now has the matched-vs-missing block-focus proof pair on the real
`00204.c` route.
`tests/backend/CMakeLists.txt` adds
`backend_cli_dump_mir_focus_block_missing_00204` and
`backend_cli_trace_mir_focus_block_missing_00204`, locking that
`--mir-focus-function stdarg --mir-focus-block missing.block` keeps the
module-level handoff summary, preserves the final `stdarg` rejection, and
reports zero matched focused BIR/prepared blocks with the stable
`no block in the focused function matched the requested MIR block focus`
footer.
With the existing `entry`-label coverage, Step 2.3 now proves that focused
summary/trace output distinguishes a matched label from a missing one without
changing route order or lane selection.

## Suggested Next

Start Step 2.4 by proving the full ladder on `00204.c`: semantic BIR facts,
prepared-stage facts, x86 route rejection class, and next-inspect guidance
should read as one deliberate backend workflow rather than isolated dumps.

## Watchouts

- The block focus contract remains diagnostic-only in both summary and trace:
  it narrows block reporting inside the focused function but does not prune
  lane attempts, route ordering, or final-rejection selection.
- `--mir-focus-block` is still intentionally gated behind
  `--mir-focus-function`; future widening should stay explicit because the
  current summary footer only makes sense in a selected-function view.
- Summary mode reports focused block counts, not per-lane block listings; that
  keeps large-case narrowing lightweight while trace remains the label-by-label
  inspection surface.

## Proof

Ran the delegated proof command exactly as assigned:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_route_debug|backend_cli_(dump_mir_00204_.*|trace_mir_00204_.*|dump_mir_focus_.*_00204|trace_mir_focus_.*_00204))$' && build/c4cll --target x86_64-unknown-linux-gnu --dump-mir --mir-focus-function stdarg --mir-focus-block missing.block tests/c/external/c-testsuite/src/00204.c && build/c4cll --target x86_64-unknown-linux-gnu --trace-mir --mir-focus-function stdarg --mir-focus-block missing.block tests/c/external/c-testsuite/src/00204.c) > test_after.log 2>&1`
and it passed. The targeted subset now includes the new
`backend_cli_*_focus_block_missing_00204` pair, and direct live output for
`00204.c` in both summary and trace mode shows the same final `stdarg`
rejection plus the stable zero-match block-focus footer when the label is
absent. Proof log path: `test_after.log`.
