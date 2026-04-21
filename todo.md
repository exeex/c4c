# Execution State

Status: Active
Source Idea Path: ideas/open/67_backend_trace_and_error_contract_for_x86_handoff.md
Source Plan Path: plan.md
Current Step ID: 2.3
Current Step Title: Add Focus Controls For Large Cases
Plan Review Counter: 2 / 6
# Current Packet

## Just Finished

Step 2.3 now exposes a second stable MIR focus seam inside a selected
function for trace inspection.
`src/apps/c4cll.cpp` accepts `--mir-focus-block <label>` for `--trace-mir`
only and requires `--mir-focus-function`, `src/backend/backend.{hpp,cpp}`
threads that block selector through backend dump options, and
`src/backend/mir/x86/codegen/route_debug.cpp` reports focused BIR/prepared
block matches inside the selected function while preserving the module-level
handoff section and the final rejection lines. `tests/backend/CMakeLists.txt`
adds `00204.c` CLI coverage for
`--trace-mir --mir-focus-function stdarg --mir-focus-block entry`, locking the
new focus-block contract plus continued visibility of the decisive `stdarg`
rejection.

## Suggested Next

Continue Step 2.3 by deciding whether the new block seam should stay
trace-only or also gain a summary-mode contract, then add one focused
multi-block proof case if the next packet needs to prove intra-function
narrowing on a function with more than one stable block label.

## Watchouts

- The new block focus is diagnostic-only and currently does not prune lane
  attempts; it narrows the block inventory inside the focused function while
  keeping route ordering and final-rejection selection unchanged.
- `--mir-focus-block` is intentionally gated behind both `--trace-mir` and
  `--mir-focus-function`; preserve that relationship unless a future packet
  broadens the user contract on purpose.
- Module-level handoff context and the decisive final rejection still print
  under both function and block focus on `00204.c`; future narrowing work must
  keep those lines visible.

## Proof

Ran the delegated proof command exactly as assigned:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_route_debug|backend_cli_(dump_mir_00204_.*|trace_mir_00204_.*|dump_mir_focus_.*_00204|trace_mir_focus_.*_00204))$' && build/c4cll --target x86_64-unknown-linux-gnu --trace-mir --mir-focus-function stdarg tests/c/external/c-testsuite/src/00204.c) > test_after.log 2>&1`
and it passed. The targeted subset now includes
`backend_cli_trace_mir_focus_block_entry_00204`, and direct live output for
`00204.c` with `--mir-focus-block entry` shows the module-level handoff
section, the final `stdarg` rejection, and matched focused BIR/prepared block
counts in the same trace.
