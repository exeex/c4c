Status: Active
Source Idea Path: ideas/open/340_aarch64_scalar_cast_stack_homed_register_source_publication.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Refresh Current First Bad Fact

# Current Packet

## Just Finished

Step 1 refreshed the current first bad fact using the supervisor-delegated
focused proof. The current tree rebuilds, both scalar-cast backend contracts
pass, and `c_testsuite_aarch64_backend_src_00143_c` passes. This classifies the
scalar cast stack-homed register source-publication owner as already-green for
the focused representative, with no live in-scope first bad fact exposed by
this packet.

## Suggested Next

Supervisor lifecycle routing: treat Step 1 as complete and decide whether this
runbook should close or be replaced, since the delegated representative and
nearby scalar-cast source-publication coverage are green.

## Watchouts

- The source idea was previously parked after scoped progress; do not assume
  the historical `00143` structured register-source diagnostic still exists.
- This packet did not expose fallthrough fixed-offset local load/store
  emission, local storage/writeback sizing, or another owner; all delegated
  focused tests passed.
- Do not change expectations, unsupported classifications, runner behavior,
  timeout policy, CTest registration, or proof-log policy.

## Proof

Command:

```sh
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_scalar_cast_records|backend_aarch64_prepared_scalar_cast_records|c_testsuite_aarch64_backend_src_00143_c)$'; } > test_after.log 2>&1
```

Result: passed. Build reported `ninja: no work to do`; CTest passed 3/3 tests:
`backend_aarch64_scalar_cast_records`,
`backend_aarch64_prepared_scalar_cast_records`, and
`c_testsuite_aarch64_backend_src_00143_c`. Proof log: `test_after.log`.
