# Execution State

Status: Active
Source Idea Path: ideas/open/67_backend_trace_and_error_contract_for_x86_handoff.md
Source Plan Path: plan.md
Current Step ID: 2.2
Current Step Title: Make X86 Rejection Diagnostics Plain And Actionable
Plan Review Counter: 3 / 6
# Current Packet

## Just Finished

Step 2.2 completed for the bounded multi-function x86 handoff rejection
surface. `src/backend/mir/x86/codegen/route_debug.cpp` now publishes a durable
module-level final rejection, final detail, and next-inspect guidance even
when the bounded multi-function lane rejects through a silent `nullopt`, so
`--dump-mir` / `--trace-mir` for
`tests/c/external/c-testsuite/src/00204.c` no longer start with a blind
module-level rejection. Focused coverage now locks the module-level route-debug
contract in both `tests/backend/backend_x86_route_debug_test.cpp` and
`tests/backend/backend_x86_handoff_boundary_multi_defined_rejection_test.cpp`.

## Suggested Next

Pick the next idea-67 packet that gives another large unsupported shape in
`00204.c` a lane-specific final rejection, with `myprintf` as the nearest
candidate because it still falls through to the ordinary per-function miss
after the module-level bounded multi-function rejection is surfaced.

## Watchouts

- The new module-level detail in `route_debug.cpp` is synthesized from the
  canonical bounded multi-function contract when the helper rejects via
  `rendered_module == nullopt`; this packet intentionally does not widen or
  refactor the actual x86 handoff implementation in
  `src/backend/mir/x86/codegen/prepared_module_emit.cpp`.
- `00204.c` now starts with an actionable bounded multi-function rejection, but
  several individual functions still end at the ordinary per-function miss, so
  route-debug remains diagnostic rather than supportive for those shapes.

## Proof

Ran the delegated proof command exactly as assigned:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_x86_(route_debug|handoff_boundary)$' && build/c4cll --target x86_64-unknown-linux-gnu --dump-mir tests/c/external/c-testsuite/src/00204.c && build/c4cll --target x86_64-unknown-linux-gnu --trace-mir tests/c/external/c-testsuite/src/00204.c > test_after.log 2>&1`
and it passed. `test_after.log` preserves the final `--trace-mir` output, and
the `--dump-mir` summary for `00204.c` now starts with:
`module-level bounded multi-function lane: rejected`,
`- module-level final rejection: bounded multi-function handoff recognized the module, but the prepared shape is outside the current x86 support`,
and
`- module-level next inspect: inspect the current x86 bounded multi-function shape support in src/backend/mir/x86/codegen/prepared_module_emit.cpp`.
