# Execution State

Status: Active
Source Idea Path: ideas/open/67_backend_trace_and_error_contract_for_x86_handoff.md
Source Plan Path: plan.md
Current Step ID: 2.4
Current Step Title: Prove The Debug Ladder Is Coherent
Plan Review Counter: 3 / 6
# Current Packet

## Just Finished

Step 2.4 now closes another real `00204.c` ladder gap: semantic and prepared
backend dumps can now be narrowed to the same focused function as the MIR
summary and trace instead of forcing a full-module read before the x86 route
surface becomes usable.
`--mir-focus-function` now applies to `--dump-bir` and `--dump-prepared-bir`,
the backend dump path trims semantic/prepared output to the requested function,
and focused prepared output keeps only that function's prepared metadata and
per-function prepare notes.
`tests/backend/CMakeLists.txt` adds honest `00204.c` CLI coverage for focused
semantic and prepared dumps so the `stdarg` handoff lane can be followed from
semantic BIR to prepared BIR to MIR without scrolling through unrelated
functions first.

## Suggested Next

Decide whether Step 2.4 is now exhausted enough to advance to Step 3. If a
new packet is still needed under idea 67, it should only proceed for a real
remaining large-case observability gap such as semantic/prepared block or
value narrowing, not for more duplication of already-readable `00204.c`
contracts.

## Watchouts

- `--mir-focus-function` now scopes semantic/prepared dumps too, but
  `--mir-focus-block` remains MIR-only; do not imply block-level BIR filtering
  exists yet.
- Focused prepared output should keep durable per-function notes and metadata,
  not allocator-noisy whole-module summaries from unrelated functions.
- If later work needs semantic/prepared block or value focus, treat that as a
  new observability packet instead of silently widening this function-only
  contract.

## Proof

Ran the delegated proof command exactly as assigned:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_route_debug|backend_cli_(dump_bir_(00204_stdarg_semantic_handoff|focus_function_filters_00204)|dump_prepared_bir_(00204_stdarg_prepared_handoff|focus_function_filters_00204)|dump_mir_focus_function_filters_00204|trace_mir_focus_function_filters_00204|dump_mir_focus_block_entry_00204|trace_mir_focus_block_entry_00204))$' && build/c4cll --target x86_64-unknown-linux-gnu --dump-bir --mir-focus-function stdarg tests/c/external/c-testsuite/src/00204.c && build/c4cll --target x86_64-unknown-linux-gnu --dump-prepared-bir --mir-focus-function stdarg tests/c/external/c-testsuite/src/00204.c) > test_after.log 2>&1`
and it passed. The subset now proves the real `00204.c` `stdarg` ladder can be
read at semantic BIR, prepared BIR, MIR summary, and MIR trace with one stable
function focus. Proof log path: `test_after.log`.
