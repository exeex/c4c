# Current Packet

Status: Active
Source Idea Path: ideas/open/359_aarch64_recursive_stack_preserved_pointer_formal_post_call_overwrite.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Stack-Preserved Pointer-Formal First Bad Fact

## Just Finished

Completed `plan.md` Step 1 refresh for idea 359. The focused AArch64 proof is
green across the external return-add smoke case and `00170`, `00181`, and
`00189`, so the old stack-preserved pointer-formal post-call overwrite is not a
live owner in this subset.

## Suggested Next

Supervisor should decide whether this absence is enough to close or retire the
runbook, or whether a plan-owner lifecycle handoff is needed for idea 359.

## Watchouts

No generated `00181` inspection was needed because the delegated focused proof
passed and did not indicate a returned overwrite. Do not chase a different
`00181` failure under this idea without a new lifecycle decision.

## Proof

Ran exactly:

```sh
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_cli_aarch64_asm_external_return_add_smoke|c_testsuite_aarch64_backend_src_001(70|81|89)_c)$'; } > test_after.log 2>&1
```

Result: passed. `test_after.log` records `ninja: no work to do.` and `100% tests
passed, 0 tests failed out of 4` for
`backend_cli_aarch64_asm_external_return_add_smoke`,
`c_testsuite_aarch64_backend_src_00170_c`,
`c_testsuite_aarch64_backend_src_00181_c`, and
`c_testsuite_aarch64_backend_src_00189_c`.
