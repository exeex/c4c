Status: Active
Source Idea Path: ideas/open/189_phase_e_cross_target_route_view_interface_reuse.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Final Route-Quality Check

# Current Packet

## Just Finished

Step 4 final route-quality check passed for the selected x86
`ConsumedPlans` boundary.

The accepted slice reuses the proven Route 6 call-use source view at one x86
call-boundary selector, keeps x86 ABI placement, wrapper-kind decisions, frame
layout, register selection, move bundles, value homes, and instruction spelling
target-owned, and does not duplicate `PreparedFunctionLookups` under a
BIR-owned name.

The interface/fallback proof is now in the default proof path:
`backend_prepared_lookup_helper` constructs a scalar named `i32` direct-call
source fixture and proves that
`find_consumed_scalar_i32_call_argument_source(...)` returns a Route 6
`ArgumentValue` record only when Route 6 facts and the prepared
`PreparedCallArgumentPlan` source id agree. It also checks that absent Route 6
facts and mismatched Route 6/prepared source ids fail closed while preserving
the prepared call-argument selector for fallback.

## Suggested Next

Plan-owner lifecycle review: the active runbook appears complete and ready for
close unless the plan owner finds that the source idea needs a follow-up open
initiative.

## Watchouts

- The default-build proof still selects only the three always-built prepared
  tests in this workspace; `backend_x86_route_debug` and
  `backend_x86_handoff_boundary` remain gated by `C4C_ENABLE_X86_BACKEND_TESTS`
  and do not appear in the selected CTest run.
- The always-built assertion is intentionally interface-level: it proves Route
  6/prepared agreement and fallback selector preservation without moving x86
  ABI placement, wrapper-kind decisions, frame layout, or instruction spelling
  into BIR.
- A reviewer found the route aligned after Step 2 and recommended Step 3
  coverage hardening rather than route reset or plan split.

## Proof

Step 3 narrow proof:

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

Final broader supervisor proof:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'
```

Result: passed, 180/180 backend tests.
