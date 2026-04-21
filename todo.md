# Execution State

Status: Active
Source Idea Path: ideas/open/67_backend_trace_and_error_contract_for_x86_handoff.md
Source Plan Path: plan.md
Current Step ID: 2.2
Current Step Title: Make X86 Rejection Diagnostics Plain And Actionable
Plan Review Counter: 5 / 6
# Current Packet

## Just Finished

Step 2.2 completed for the single-block void call-wrapper rejection surface.
`src/backend/mir/x86/codegen/route_debug.cpp` now classifies unsupported
single-block void helpers with live call sequences as their own rejection
family, including lane-specific final rejection, detail, and next-inspect
guidance, so `stdarg` in `tests/c/external/c-testsuite/src/00204.c` no longer
falls back to the plain per-function miss. Acceptance proof now uses honest
CLI coverage in `tests/backend/CMakeLists.txt` for
`backend_cli_dump_mir_00204_stdarg_rejection` and
`backend_cli_trace_mir_00204_stdarg_rejection`, so the packet stays anchored
to the real `00204.c` route instead of a reduced fixture that would drift into
an already-supported helper surface.

## Suggested Next

Pick the next idea-67 packet that gives another remaining unsupported wrapper
or helper family in `00204.c` a lane-specific final rejection instead of the
generic per-function miss, now that the module-level, `myprintf`-class, and
`stdarg`-class rejection surfaces are all exposed through honest CLI coverage.

## Watchouts

- The new single-block void call-sequence rejection is intentionally
  diagnostic-only: it classifies unsupported wrapper-style void helpers and
  points inspection to
  `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`; it does not
  widen or refactor the actual x86 helper implementation.
- Keep Step 2.2 proof anchored on `tests/c/external/c-testsuite/src/00204.c`;
  any reduced coverage here should remain a route-debug contract fixture, not
  the only acceptance signal.
- `00204.c` still contains other unsupported shapes that end at the ordinary
  per-function miss, so route-debug remains incomplete beyond the module-level,
  `myprintf`-class helper, and `stdarg`-class wrapper diagnostics landed here.

## Proof

Ran the delegated proof command exactly as assigned:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_(route_debug|handoff_boundary)|backend_cli_(dump_mir_00204_(myprintf|stdarg)_rejection|trace_mir_00204_(myprintf|stdarg)_rejection))$' && build/c4cll --target x86_64-unknown-linux-gnu --dump-mir tests/c/external/c-testsuite/src/00204.c && build/c4cll --target x86_64-unknown-linux-gnu --trace-mir tests/c/external/c-testsuite/src/00204.c) > test_after.log 2>&1`
and it passed. `test_after.log` preserves the final `--trace-mir` output, and
the named CLI tests now lock the `myprintf` and `stdarg` rejection surfaces
from the real `00204.c` input.
