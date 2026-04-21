# Execution State

Status: Active
Source Idea Path: ideas/open/67_backend_trace_and_error_contract_for_x86_handoff.md
Source Plan Path: plan.md
Current Step ID: 2.2.3
Current Step Title: Eliminate Remaining Generic Per-Function Misses In The Motivating Case
Plan Review Counter: 2 / 6
# Current Packet

## Just Finished

Step 2.2.3 progressed for the remaining homogeneous-float-aggregate helper
surface in `tests/c/external/c-testsuite/src/00204.c`.
`src/backend/mir/x86/codegen/route_debug.cpp` now classifies single-block void
helpers that forward floating aggregate arguments through byval/pointer
wrappers into a direct variadic extern call as their own lane-specific final
rejection family, with stable summary, trace detail, and next-inspect
guidance. That removes the generic per-function miss for the `fa_hfa*`
helpers plus the shared `fa3` and `fa4` wrappers without widening backend
support. Acceptance proof now includes honest CLI coverage in
`tests/backend/CMakeLists.txt` for
`backend_cli_dump_mir_00204_hfa_rejection` and
`backend_cli_trace_mir_00204_hfa_rejection`, plus reduced route-debug fixture
coverage for the shared floating-aggregate helper shape.

## Suggested Next

Continue at Step 2.2.3 by identifying the next remaining meaningful
`00204.c` x86 rejection family that still collapses into the generic
per-function miss. After the variadic, `ashr`, immediate-helper return, and
floating-aggregate helper lanes, the remaining generic misses are concentrated
in other aggregate-heavy wrappers such as `arg`, `opi`, and nearby
non-trivial aggregate forwarding helpers that still terminate as plain route
misses.

## Watchouts

- The new floating-aggregate helper rejection is intentionally diagnostic-only:
  it classifies unsupported aggregate-helper shapes and points inspection to
  `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`; it does not
  widen or refactor the actual x86 aggregate helper implementation.
- Keep Step 2.2 proof anchored on `tests/c/external/c-testsuite/src/00204.c`;
  any reduced coverage here should remain a route-debug contract fixture, not
  the only acceptance signal.
- `00204.c` still contains other unsupported shapes that end at the ordinary
  per-function miss, so route-debug remains incomplete beyond the module-level,
  `myprintf`-class helper, `stdarg`-class wrapper, `ashr`-class return helper,
  immediate-helper return helper, and floating-aggregate helper diagnostics
  landed here.

## Proof

Ran the delegated proof command exactly as assigned:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_(route_debug|handoff_boundary)|backend_cli_(dump_mir_00204_(myprintf|stdarg|asrl|i64_immediate|hfa)_rejection|trace_mir_00204_(myprintf|stdarg|asrl|i64_immediate|hfa)_rejection))$' && build/c4cll --target x86_64-unknown-linux-gnu --dump-mir tests/c/external/c-testsuite/src/00204.c && build/c4cll --target x86_64-unknown-linux-gnu --trace-mir tests/c/external/c-testsuite/src/00204.c) > test_after.log 2>&1`
and it passed. `test_after.log` preserves the final `--trace-mir` output, and
the named CLI tests now lock the `myprintf`, `stdarg`, `asrl`,
immediate-helper, and new HFA-family rejection surfaces from the real
`00204.c` input.
