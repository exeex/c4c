Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select One Retained Metadata Authority Leak

# Current Packet

## Just Finished

Step 5 closure review rejected closure for the byval register-lane authority
removal checkpoint. The source idea remains open because target-local
`calls*.cpp` code still rederives call-boundary decisions from retained
`CallInst::arg_abi` or `CallInst::arg_types`:

- `calls_common.cpp`: `outgoing_stack_argument_bytes` uses
  `call.arg_abi[argument.arg_index]` for outgoing stack argument sizing.
- `calls_dispatch_bridge.cpp`: `call_argument_allows_local_aggregate_address_publication`
  uses retained `arg_abi`, `arg_types`, and `args` shape to decide publication
  eligibility.
- `calls_argument_sources.cpp`: `call_argument_is_pointer`,
  `call_argument_is_byval_copy`, and
  `call_argument_allows_local_frame_address_publication` use retained
  `arg_types` and `arg_abi`.
- `calls_byval_aggregates.cpp`: byval size and indirect-register predicates
  still inspect retained `arg_abi` shape.

The accepted canonical `test_before.log` records
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_')`
passing 162/162 backend tests. No `test_after.log` is present, so the
close-time regression guard is not acceptance-grade even if source completion
were otherwise true.

## Suggested Next

Resume execution at Step 1 of `plan.md`: select one surviving retained
metadata authority leak, map it to an existing prepared fact or a precise
missing-prepared-fact blocker, and record the selected proof scope before
implementation.

## Watchouts

- Do not close the source idea from the current state.
- Do not touch `ideas/open/03_dispatch_responsibility_reduction.md`.
- Treat `CallInst` reads as acceptable only for identity validation,
  diagnostics, or emission context; publication, stack sizing, and byval shape
  decisions need prepared authority.
- If a prepared fact is missing, stop and record the blocker instead of
  rebuilding the decision locally.

## Proof

No new build was required by the Step 5 lifecycle review. Existing
`test_before.log` shows `^backend_` passing 162/162. Closure was rejected on
source-completion grounds and because `test_after.log` is absent for a
close-time before/after guard.
