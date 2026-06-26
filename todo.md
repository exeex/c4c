Status: Active
Source Idea Path: ideas/open/386_rv64_object_route_same_module_byval_aggregate_call_args.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Broader Guard And Closure Review

# Current Packet

## Just Finished

Step 4: Rerun Representative And Route Next Boundary completed the
representative rerun for `src/20030914-2.c`.

Supervisor-provided proof showed
`llvm_gcc_c_torture_src_20030914_2_c` passes through the RV64 object route.
The representative rerun did not expose a new distinct boundary, so execution
can move to Step 5 broader guard and closure review.

## Suggested Next

Run Step 5 validation and closure review: choose the appropriate broader guard
for the repaired RV64 byval aggregate call-argument route, then decide whether
the active plan can close or needs lifecycle follow-up.

## Watchouts

- Do not reopen prepared global-symbol publication; idea 384 closed that
  boundary.
- Do not treat closed byval parameter-home idea 370 as owning this callsite
  movement boundary.
- Keep aggregate `va_arg` and non-register parameter-home work routed to ideas
  371 and 374 respectively.
- Do not modify or rely on unrelated EV64 idea 385 for this lifecycle route.
- Step 4 representative rerun is now recorded as passing; no additional focused
  boundary was exposed by `src/20030914-2.c`.
- `tests/backend/mir/backend_riscv_object_emission_test.cpp` is the existing
  test file for the delegated `backend_riscv_object_emission` proof target,
  even though the packet named `tests/backend/bir` for optional focused tests.
- Keep future RV64 byval fixes on prepared `StackCopy` facts; do not add
  testcase-name matching or emitter-side stack-object suffix reconstruction.
- The dump expectation is intentionally stricter than the old failed snippet:
  it requires the explicit local-frame source selection and the byval stack-copy
  facts instead of reverting to `prior_preservation`.

## Proof

Supervisor-provided proof command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^llvm_gcc_c_torture_src_20030914_2_c$'
```

Result: passed 1/1. Canonical proof log: `test_after.log`.
