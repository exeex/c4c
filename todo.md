Status: Active
Source Idea Path: ideas/open/189_phase_e_cross_target_route_view_interface_reuse.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Prove Interface Reuse and Fallback Coverage

# Current Packet

## Just Finished

Step 3 made the x86 Route 6-through-`ConsumedPlans` interface and fallback
coverage run in the default proof path by adding an always-built assertion to
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp`.

The new default-path coverage constructs a scalar named `i32` direct-call
source fixture and proves that
`find_consumed_scalar_i32_call_argument_source(...)` returns a Route 6
`ArgumentValue` record only when Route 6 facts and the prepared
`PreparedCallArgumentPlan` source id agree. The same assertion also checks that
absent Route 6 facts and mismatched Route 6/prepared source ids fail closed
while preserving the prepared call-argument selector for fallback.

## Suggested Next

Supervisor review/acceptance for Step 3, including whether the current runbook
is now ready for lifecycle close or needs a final broader validation packet.

## Watchouts

- The default-build proof still selects only the three always-built prepared
  tests in this workspace; `backend_x86_route_debug` and
  `backend_x86_handoff_boundary` remain gated by `C4C_ENABLE_X86_BACKEND_TESTS`
  and do not appear in the selected CTest run.
- The always-built assertion is intentionally interface-level: it proves Route
  6/prepared agreement and fallback selector preservation without moving x86
  ABI placement, wrapper-kind decisions, frame layout, or instruction spelling
  into BIR.

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
