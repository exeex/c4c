# Execution State

Status: Active
Source Idea Path: ideas/open/67_backend_trace_and_error_contract_for_x86_handoff.md
Source Plan Path: plan.md
Current Step ID: 2.4
Current Step Title: Prove The Debug Ladder Is Coherent
Plan Review Counter: 1 / 6
# Current Packet

## Just Finished

Step 2.4 now proves the full `00204.c` debug ladder reads as one backend
workflow instead of isolated dumps.
`tests/backend/CMakeLists.txt` adds
`backend_cli_dump_bir_00204_stdarg_semantic_handoff` and
`backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff`, locking that
`--dump-bir` exposes the semantic `stdarg` same-module call-wrapper shape
before routing, `--dump-prepared-bir` carries that same function through the
prepared-module metadata/control-flow/value-location surfaces, and the existing
`--dump-mir` / `--trace-mir` `stdarg` coverage continues the ladder into the
x86 rejection class and next-inspect guidance.

## Suggested Next

Close the remaining observability gap inside Step 2.4 by deciding whether the
ladder also needs one narrower `00204.c` proof that ties a prepared block label
from `--dump-prepared-bir` directly to the existing focused `--dump-mir` /
`--trace-mir` `entry` block assertions.

## Watchouts

- The new ladder proof intentionally anchors on stable `stdarg` shape markers
  and prepared metadata headings instead of exact frame-slot numbering or
  full-value-location dumps, which would overfit allocator churn rather than
  the debug contract.
- The current `00204.c` ladder now proves stage-to-stage continuity for one
  concrete unsupported helper family; widening it to more functions should stay
  deliberate so the packet does not turn into bulk expectation duplication.
- Focused MIR coverage still starts at the MIR/debug layer, not at prepared BIR
  block labels; if the route needs a stricter block-identity story, add that as
  a separate packet rather than overloading the ladder smoke tests.

## Proof

Ran the delegated proof command exactly as assigned:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_route_debug|backend_cli_(dump_bir_00204_.*|dump_prepared_bir_00204_.*|dump_mir_00204_.*|trace_mir_00204_.*|dump_mir_focus_.*_00204|trace_mir_focus_.*_00204))$' && build/c4cll --target x86_64-unknown-linux-gnu --dump-bir tests/c/external/c-testsuite/src/00204.c && build/c4cll --target x86_64-unknown-linux-gnu --dump-prepared-bir tests/c/external/c-testsuite/src/00204.c && build/c4cll --target x86_64-unknown-linux-gnu --dump-mir tests/c/external/c-testsuite/src/00204.c && build/c4cll --target x86_64-unknown-linux-gnu --trace-mir tests/c/external/c-testsuite/src/00204.c) > test_after.log 2>&1`
and it passed. The subset now proves the ladder in order on the real `00204.c`
input: semantic BIR `stdarg` call-wrapper shape, prepared-BIR metadata/control
flow for the same function, then MIR summary/trace rejection and next-inspect
guidance for that prepared shape. Proof log path: `test_after.log`.
