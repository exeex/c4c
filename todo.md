Status: Active
Source Idea Path: ideas/open/189_phase_e_cross_target_route_view_interface_reuse.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Thread the Proven Route View Through the Wrapper

# Current Packet

## Just Finished

Step 2 threaded the Route 6 call-use source view through the x86
`ConsumedPlans` call-boundary wrapper for scalar named `i32` call arguments.
`ConsumedPlans` now carries an optional `bir::Route6CallUseSourceIndex`, exposes
`shared_route6_call_use_source_index()`, and adds
`find_consumed_scalar_i32_call_argument_source(...)`, which only returns a Route
6 argument source record when the Route 6 source is available, the source class
is `ArgumentValue`, and the Route 6 source value id matches the prepared
`PreparedCallArgumentPlan` source id.

The x86 direct-call wrappers in `src/backend/mir/x86/module/module.cpp` now
pass that optional Route 6 source into scalar call-argument rendering while
still requiring prepared BeforeCall bundles for ABI destinations and prepared
value homes for final x86 register moves. When Route 6 facts are absent, the
helper fails closed and the existing prepared call-plan/storage path emits the
same assembly.

Focused coverage in
`tests/backend/bir/backend_x86_handoff_boundary_direct_extern_call_test.cpp`
checks that a scalar `printf` argument source threads through `ConsumedPlans`
and that clearing Route 6 call-argument source facts preserves the prepared
call-argument selector and unchanged fallback assembly.

## Suggested Next

Delegate Step 3 to make the x86 Route 6-through-`ConsumedPlans` coverage
acceptance-grade: either enable and run the focused x86 handoff-boundary test
path in the standard proof route, or add an always-built backend test that
observes the same interface reuse and fallback behavior without moving ABI
placement, wrapper-kind decisions, frame layout, or instruction spelling out of
x86.

## Watchouts

- The current Route 6 selector is intentionally narrow: named scalar `i32`
  arguments whose Route 6 source kind is `ArgumentValue` and whose source value
  id agrees with the prepared call-argument plan.
- The x86 wrappers still require prepared move bundles and value homes before
  emitting; do not replace those target-owned ABI/storage checks with Route 6
  facts.
- `clang-format` was not available in this container, so formatting was kept
  manual.
- The delegated CTest regex selected the three prepared tests shown in
  `test_after.log`; `backend_x86_route_debug` and
  `backend_x86_handoff_boundary` did not appear in the selected CTest run.
- A supervisor x86-enabled build of `backend_x86_handoff_boundary_test`
  compiled the modified direct-extern test object, but the aggregate target was
  blocked by unrelated existing compile errors in
  `backend_x86_handoff_boundary_joined_branch_test.cpp`.

## Proof

Ran the supervisor-selected proof exactly, with combined output written to
`test_after.log`:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_x86_route_debug|backend_x86_handoff_boundary)$'
```

Result: passed. `test_after.log` shows a successful build and 3/3 selected
CTest tests passing:
`backend_prepared_lookup_helper`, `backend_prepare_liveness`, and
`backend_prepare_frame_stack_call_contract`.

Supervisor validation:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

Result: passed with no new failures.

Additional supervisor compile probe:

```sh
cmake -S . -B build-x86 -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_BACKEND_TESTS=ON -DC4C_ENABLE_X86_BACKEND_TESTS=ON
cmake --build build-x86 --target backend_x86_handoff_boundary_test
```

Result: the modified
`backend_x86_handoff_boundary_direct_extern_call_test.cpp` object compiled, but
the aggregate target stopped on unrelated stale call-site errors in
`backend_x86_handoff_boundary_joined_branch_test.cpp`.
