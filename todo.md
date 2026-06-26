Status: Active
Source Idea Path: ideas/open/386_rv64_object_route_same_module_byval_aggregate_call_args.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Broader Guard And Closure Review

# Current Packet

## Just Finished

Step 5: Broader Guard And Closure Review completed broader validation for the
repaired RV64 byval aggregate call-argument route.

Supervisor-provided full-suite proof passed 3353/3353. No expectation or
allowlist downgrade was accepted, and this active plan is ready for
plan-owner closure review.

## Suggested Next

Ask the plan owner to perform closure review for the active plan and decide
whether the linked source idea can close or needs lifecycle follow-up.

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
- Step 5 accepted validation results only; it did not accept expectation
  rewrites, allowlist changes, or testcase-shaped downgrades as progress.

## Proof

Supervisor-provided Step 5 proof command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure
```

Result: passed 3353/3353.

Earlier supervisor proofs after regression repair also passed:

```sh
ctest --test-dir build -j --output-on-failure -R '^backend_'
```

Result: passed 324/324.

```sh
ctest --test-dir build -j --output-on-failure -R '^llvm_gcc_c_torture_src_20030914_2_c$'
```

Result: passed 1/1.

Canonical proof log: `test_after.log`.
