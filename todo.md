# Execution State

Status: Active
Source Idea Path: ideas/open/67_backend_trace_and_error_contract_for_x86_handoff.md
Source Plan Path: plan.md
Current Step ID: 2.2.3
Current Step Title: Eliminate Remaining Generic Per-Function Misses In The Motivating Case
Plan Review Counter: 1 / 6
# Current Packet

## Just Finished

Step 2.2.3 progressed for the shared single-block i64 immediate return-helper
rejection surface. `src/backend/mir/x86/codegen/route_debug.cpp` now
classifies unsupported single-block i64 add/subtract/or/xor/logical-shift
helpers with one immediate operand as their own lane-specific rejection
family, with stable final rejection, detail, and next-inspect guidance, so
`sublp0`, `sublp4095`, `addp12345`, `negl`, `rsbl123`, `orrl0`, `orr0xf0`,
`eor0xf0`, `lsll1`, `lsll63`, `lsrl1`, and `lsrl63` in
`tests/c/external/c-testsuite/src/00204.c` no longer fall back to the generic
per-function miss. Acceptance proof now includes honest CLI coverage in
`tests/backend/CMakeLists.txt` for
`backend_cli_dump_mir_00204_i64_immediate_rejection` and
`backend_cli_trace_mir_00204_i64_immediate_rejection`, plus route-debug
fixture coverage for the shared family shape instead of a named-case shortcut.

## Suggested Next

Continue at Step 2.2.3 by identifying the next remaining meaningful
`00204.c` x86 rejection family that still collapses into the generic
per-function miss. The largest visible clusters after the variadic, `ashr`,
and immediate-helper return lanes are the homogeneous-float-aggregate helper
families (`fa_hfa*`, `fa3`, `fa4`) and other aggregate-heavy wrappers that
still terminate as plain route misses.

## Watchouts

- The new single-block i64 immediate return-helper rejection is intentionally
  diagnostic-only: it classifies unsupported return-helper shapes and points
  inspection to
  `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`; it does not
  widen or refactor the actual x86 helper implementation.
- Keep Step 2.2 proof anchored on `tests/c/external/c-testsuite/src/00204.c`;
  any reduced coverage here should remain a route-debug contract fixture, not
  the only acceptance signal.
- `00204.c` still contains other unsupported shapes that end at the ordinary
  per-function miss, so route-debug remains incomplete beyond the module-level,
  `myprintf`-class helper, `stdarg`-class wrapper, `ashr`-class return helper,
  and immediate-helper return diagnostics landed here.

## Proof

Ran the delegated proof command exactly as assigned:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_(route_debug|handoff_boundary)|backend_cli_(dump_mir_00204_(myprintf|stdarg|asrl|i64_immediate)_rejection|trace_mir_00204_(myprintf|stdarg|asrl|i64_immediate)_rejection))$' && build/c4cll --target x86_64-unknown-linux-gnu --dump-mir tests/c/external/c-testsuite/src/00204.c && build/c4cll --target x86_64-unknown-linux-gnu --trace-mir tests/c/external/c-testsuite/src/00204.c) > test_after.log 2>&1`
and it passed. `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`
also passed, with the backend subset moving from 8 to 10 passing checks and no
new failures. `test_after.log` preserves the final `--trace-mir` output, and
the named CLI tests now lock the `myprintf`, `stdarg`, `asrl`, and new
immediate-helper rejection surfaces from the real `00204.c` input.
