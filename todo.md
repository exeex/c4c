Status: Active
Source Idea Path: ideas/open/340_aarch64_scalar_cast_stack_homed_register_source_publication.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Refresh Current Scalar-Cast Proof

# Current Packet

## Just Finished

Step 1 of `plan.md` refreshed the current scalar-cast stack-homed
register-source publication proof. The focused subset passed, so there is no
live current first bad fact to classify as in scope for idea 340.

## Suggested Next

Supervisor route to Step 3 for the close/deactivate decision. No implementation
packet is suggested from this proof because no in-scope scalar-cast
source-publication failure is live.

## Watchouts

- Do not special-case `00143`, `%t76`, `%t81`, value ids 308 or 388, `x13`,
  block 16, instruction 158, or one diagnostic string.
- Do not reopen fallthrough fixed-offset local load/store, frame layout,
  compare lowering, runtime value correctness, or expectation policy without
  fresh first-bad-fact evidence and lifecycle routing.
- Leave `ideas/open/340_aarch64_scalar_cast_stack_homed_register_source_publication.md`
  unchanged unless closure, deactivation, or source-intent repair requires it.

## Proof

Command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_scalar_cast_records|backend_aarch64_prepared_scalar_cast_records|c_testsuite_aarch64_backend_src_00143_c)$'
```

Result: passed, 3/3. The build reported `ninja: no work to do`.

Proof log: `test_after.log`.
