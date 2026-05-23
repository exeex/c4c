Status: Active
Source Idea Path: ideas/open/340_aarch64_scalar_cast_stack_homed_register_source_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Scalar-Cast Publication Proof

# Current Packet

## Just Finished

Activation created this executor-compatible lifecycle state for Step 1.

## Suggested Next

Run Step 1 from `plan.md`: build the default preset and execute the focused
scalar-cast publication subset:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_scalar_cast_records|backend_aarch64_prepared_scalar_cast_records|c_testsuite_aarch64_backend_src_00143_c)$'
```

Classify any failure by first bad fact before deciding whether Step 2 has live
implementation work or whether the plan should return to lifecycle close
consideration.

## Watchouts

- Do not special-case `00143`, value ids, registers, block names, diagnostic
  strings, or selected instruction indexes.
- Do not change expectations, unsupported classifications, runner behavior,
  timeout policy, CTest registration, or proof-log behavior.
- If the refreshed failure is not scalar-cast source publication, record the
  classification instead of widening this plan.

## Proof

Not run during activation. First proof belongs to Step 1 execution.
