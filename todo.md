# Execution State

Status: Active
Source Idea Path: ideas/open/67_backend_trace_and_error_contract_for_x86_handoff.md
Source Plan Path: plan.md
Current Step ID: 2.3
Current Step Title: Add Focus Controls For Large Cases
Plan Review Counter: 3 / 6
# Current Packet

## Just Finished

Step 2.3 now gives `--dump-mir` the same block-focus selector contract as
`--trace-mir` when `--mir-focus-function` is present.
`src/apps/c4cll.cpp` broadens the CLI gate/help text so
`--mir-focus-block <label>` is accepted under summary mode,
`src/backend/mir/x86/codegen/route_debug.cpp` makes summary output count and
report focused BIR/prepared blocks in the selected function, and
`tests/backend/CMakeLists.txt` adds `00204.c` coverage for
`--dump-mir --mir-focus-function stdarg --mir-focus-block entry`, proving that
module-level handoff context and the final `stdarg` rejection stay visible
while the summary narrows block accounting to the requested block.

## Suggested Next

If Step 2.3 needs one more proving packet, add a stable multi-block case that
shows summary/trace block focus distinguishing a matched block from a missing
label inside the same function without changing route order or lane selection.

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
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_route_debug|backend_cli_(dump_mir_00204_.*|trace_mir_00204_.*|dump_mir_focus_.*_00204|trace_mir_focus_.*_00204))$' && build/c4cll --target x86_64-unknown-linux-gnu --dump-mir --mir-focus-function stdarg --mir-focus-block entry tests/c/external/c-testsuite/src/00204.c) > test_after.log 2>&1`
and it passed. The targeted subset now includes
`backend_cli_dump_mir_focus_block_entry_00204`, and direct live output for
`00204.c` in summary mode shows the module-level handoff section, the final
`stdarg` rejection, and matched focused BIR/prepared block counts in the same
dump. Proof log path: `test_after.log`.
