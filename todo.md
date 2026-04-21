# Execution State

Status: Active
Source Idea Path: ideas/open/67_backend_trace_and_error_contract_for_x86_handoff.md
Source Plan Path: plan.md
Current Step ID: 2.2.3
Current Step Title: Eliminate Remaining Generic Per-Function Misses In The Motivating Case
Plan Review Counter: 5 / 6
# Current Packet

## Just Finished

Step 2.2.3 progressed for the remaining floating-aggregate wrapper/helper
surfaces in `tests/c/external/c-testsuite/src/00204.c`.
`src/backend/mir/x86/codegen/route_debug.cpp` now treats the late `fa_hfa23+`
pointer-wrapper forms as the existing single-block floating aggregate call
helper family even when the pointer parameter no longer carries explicit byval
size metadata, and it classifies `fr_hfa*` same-module floating aggregate
copyout helpers as their own lane-specific final rejection family. That
removes the generic per-function miss for the remaining `fa_hfa*` / `fr_hfa*`
surfaces without widening backend support or keying off testcase names.
Acceptance proof now includes updated CLI coverage in
`tests/backend/CMakeLists.txt` for
`backend_cli_dump_mir_00204_hfa_rejection` and
`backend_cli_trace_mir_00204_hfa_rejection`, plus reduced route-debug fixture
coverage in `tests/backend/backend_x86_route_debug_test.cpp` for the
pointer-wrapper and sret-copyout floating aggregate shapes.

## Suggested Next

Continue at Step 2.2.3 by identifying the next remaining meaningful
`00204.c` x86 rejection family that still collapses into the generic
per-function miss after the floating-aggregate wrapper/helper packet. The most
visible remaining generic misses are now concentrated in unrelated helper
families outside `fa_hfa*` / `fr_hfa*`.

## Watchouts

- The new floating-aggregate classifications are intentionally diagnostic-only:
  they point inspection to
  `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp` but do not widen
  or refactor the actual x86 floating aggregate helper or return-helper
  implementation.
- The `fa_hfa23+` route stays shape-based rather than name-based: it keys off
  floating loads from pointer parameters into a direct variadic extern call,
  including the late `ptr %p.a` forms that lost explicit byval-size metadata.
- The `fr_hfa*` route stays shape-based rather than name-based: it keys off a
  single-block floating same-module global load plus addressed copyout stores,
  not specific helper names.
- Keep Step 2.2 proof anchored on `tests/c/external/c-testsuite/src/00204.c`;
  any reduced coverage here should remain a route-debug contract fixture, not
  the only acceptance signal.
- `00204.c` still contains other unsupported shapes that end at the ordinary
  per-function miss, so route-debug remains incomplete beyond the module-level,
  `myprintf`-class helper, `stdarg`-class wrapper, `ashr`-class return helper,
  immediate-helper return helper, aggregate-forwarding wrapper, and
  same-module scalar call-wrapper diagnostics landed here.

## Proof

Ran the delegated proof command exactly as assigned:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_(route_debug|handoff_boundary_multi_defined_call)|backend_cli_(dump_mir_00204_hfa_rejection|trace_mir_00204_hfa_rejection))$' && build/c4cll --target x86_64-unknown-linux-gnu --dump-mir tests/c/external/c-testsuite/src/00204.c && build/c4cll --target x86_64-unknown-linux-gnu --trace-mir tests/c/external/c-testsuite/src/00204.c) > test_after.log 2>&1`
and it passed. `test_after.log` preserves the final `--trace-mir` output, and
the named CLI tests now lock the floating-aggregate helper and sret-copyout
rejection surfaces from the real `00204.c` input.
