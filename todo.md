# Execution State

Status: Active
Source Idea Path: ideas/open/67_backend_trace_and_error_contract_for_x86_handoff.md
Source Plan Path: plan.md
Current Step ID: 2.2.3
Current Step Title: Eliminate Remaining Generic Per-Function Misses In The Motivating Case
Plan Review Counter: 4 / 6
# Current Packet

## Just Finished

Step 2.2.3 progressed for the remaining `opi()`-class same-module call-wrapper
surface in `tests/c/external/c-testsuite/src/00204.c`.
`src/backend/mir/x86/codegen/route_debug.cpp` now classifies single-block void
wrappers that keep a local-slot scratch value live across repeated same-module
scalar helper calls, width-adjusting casts, and same-module sink wrappers as
their own lane-specific final rejection family. That removes the generic
per-function miss for `opi()` without widening backend support or keying off
testcase names.
Acceptance proof now includes honest CLI coverage in
`tests/backend/CMakeLists.txt` for
`backend_cli_dump_mir_00204_opi_rejection` and
`backend_cli_trace_mir_00204_opi_rejection`, plus reduced route-debug fixture
coverage in `tests/backend/backend_x86_route_debug_test.cpp` for the shared
same-module scalar call-wrapper family.

## Suggested Next

Continue at Step 2.2.3 by identifying the next remaining meaningful
`00204.c` x86 rejection family that still collapses into the generic
per-function miss after the `opi()` call-wrapper lane. The most visible
remaining generic misses are now concentrated in other unsupported helper
families such as the remaining HFA call surfaces and nearby scalar wrappers
that still terminate as plain route misses.

## Watchouts

- The new `opi()` rejection is intentionally diagnostic-only: it classifies
  unsupported same-module scalar call-wrapper families and points inspection to
  `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`; it does not
  widen or refactor the actual x86 same-module helper implementation.
- The recognizer is shape-based rather than name-based: it looks for repeated
  same-module scalar helper-to-sink wrapper pairs inside a single-block void
  wrapper with local-slot scratch state. Future packets in this area should
  preserve that route and avoid testcase-shaped helper-name matching.
- Keep Step 2.2 proof anchored on `tests/c/external/c-testsuite/src/00204.c`;
  any reduced coverage here should remain a route-debug contract fixture, not
  the only acceptance signal.
- `00204.c` still contains other unsupported shapes that end at the ordinary
  per-function miss, so route-debug remains incomplete beyond the module-level,
  `myprintf`-class helper, `stdarg`-class wrapper, `ashr`-class return helper,
  immediate-helper return helper, floating-aggregate helper,
  aggregate-forwarding wrapper, and same-module scalar call-wrapper
  diagnostics landed here.

## Proof

Ran the delegated proof command exactly as assigned:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_(route_debug|handoff_boundary)|backend_cli_(dump_mir_00204_opi_rejection|trace_mir_00204_opi_rejection))$' && build/c4cll --target x86_64-unknown-linux-gnu --dump-mir tests/c/external/c-testsuite/src/00204.c && build/c4cll --target x86_64-unknown-linux-gnu --trace-mir tests/c/external/c-testsuite/src/00204.c) > test_after.log 2>&1`
and it passed. `test_after.log` preserves the final `--trace-mir` output, and
the named CLI tests now lock the new `opi()` rejection surface from the real
`00204.c` input.
