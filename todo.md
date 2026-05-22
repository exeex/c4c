Status: Active
Source Idea Path: ideas/open/340_aarch64_scalar_cast_stack_homed_register_source_publication.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Refresh Current Scalar-Cast Proof

# Current Packet

## Just Finished

Lifecycle activation created this execution state for Step 1 of `plan.md`.

## Suggested Next

Run the focused scalar-cast refresh proof:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_scalar_cast_records|backend_aarch64_prepared_scalar_cast_records|c_testsuite_aarch64_backend_src_00143_c)$'
```

If all targets pass, record the proof here and route to Step 3 for a
close/deactivate decision. If any target fails, classify whether the first bad
fact is the old scalar-cast stack-homed register source-publication owner
before editing code.

## Watchouts

- Do not special-case `00143`, `%t76`, `%t81`, value ids 308 or 388, `x13`,
  block 16, instruction 158, or one diagnostic string.
- Do not reopen fallthrough fixed-offset local load/store, frame layout,
  compare lowering, runtime value correctness, or expectation policy without
  fresh first-bad-fact evidence and lifecycle routing.
- Leave `ideas/open/340_aarch64_scalar_cast_stack_homed_register_source_publication.md`
  unchanged unless closure, deactivation, or source-intent repair requires it.

## Proof

No proof has been run for this active packet yet.
