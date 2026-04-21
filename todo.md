# Execution State

Status: Active
Source Idea Path: ideas/open/67_backend_trace_and_error_contract_for_x86_handoff.md
Source Plan Path: plan.md
Current Step ID: 2.2.3
Current Step Title: Eliminate Remaining Generic Per-Function Misses In The Motivating Case
Plan Review Counter: 3 / 6
# Current Packet

## Just Finished

Step 2.2.3 progressed for the remaining aggregate-forwarding wrapper surface in
`tests/c/external/c-testsuite/src/00204.c`.
`src/backend/mir/x86/codegen/route_debug.cpp` now classifies single-block void
wrappers that mix a direct extern preamble with same-module aggregate calls as
their own lane-specific final rejection family, with stable summary, trace
detail, and next-inspect guidance. That removes the generic per-function miss
for `arg()` without widening backend support or keying off testcase names.
The follow-up fix in this packet tightened the recognizer to use both
callee-side pointer metadata and call-site aggregate ABI metadata, so raw
`--dump-mir` and raw `--trace-mir` now agree on the `arg()` rejection even
when the later calls in the wrapper family are plain pointer aggregate
forwards instead of explicit `byval` callee parameters.
Acceptance proof now includes honest CLI coverage in
`tests/backend/CMakeLists.txt` for
`backend_cli_dump_mir_00204_arg_rejection` and
`backend_cli_trace_mir_00204_arg_rejection`, plus reduced route-debug fixture
coverage for the shared aggregate-forwarding wrapper shape.

## Suggested Next

Continue at Step 2.2.3 by identifying the next remaining meaningful
`00204.c` x86 rejection family that still collapses into the generic
per-function miss. After the variadic, `ashr`, immediate-helper return,
floating-aggregate helper, and aggregate-forwarding wrapper lanes, the
remaining generic misses are concentrated in other wrappers such as `opi` and
nearby non-trivial helper sequences that still terminate as plain route
misses.

## Watchouts

- The new aggregate-forwarding wrapper rejection is intentionally
  diagnostic-only: it classifies unsupported direct-extern-plus-same-module
  aggregate-call wrappers and points inspection to
  `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`; it does not
  widen or refactor the actual x86 aggregate-call implementation.
- The earlier inconsistency came from the recognizer only trusting callee-side
  `byval` metadata; the real `arg()` wrapper family also contains aggregate
  pointer calls whose aggregate shape is only visible through call-site ABI
  metadata, so future packets in this area should check both surfaces.
- Keep Step 2.2 proof anchored on `tests/c/external/c-testsuite/src/00204.c`;
  any reduced coverage here should remain a route-debug contract fixture, not
  the only acceptance signal.
- `00204.c` still contains other unsupported shapes that end at the ordinary
  per-function miss, so route-debug remains incomplete beyond the module-level,
  `myprintf`-class helper, `stdarg`-class wrapper, `ashr`-class return helper,
  immediate-helper return helper, floating-aggregate helper, and
  aggregate-forwarding wrapper diagnostics landed here.

## Proof

Ran the delegated proof command exactly as assigned:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_(route_debug|handoff_boundary)|backend_cli_(dump_mir_00204_arg_rejection|trace_mir_00204_arg_rejection))$' && build/c4cll --target x86_64-unknown-linux-gnu --dump-mir tests/c/external/c-testsuite/src/00204.c && build/c4cll --target x86_64-unknown-linux-gnu --trace-mir tests/c/external/c-testsuite/src/00204.c) > test_after.log 2>&1`
and it passed. `test_after.log` preserves the final `--trace-mir` output, and
the named CLI tests now lock the new `arg()` rejection surface from the real
`00204.c` input.
