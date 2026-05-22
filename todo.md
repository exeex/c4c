Status: Active
Source Idea Path: ideas/open/327_aarch64_fixed_formal_entry_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh The Current Fixed-Formal First Bad Fact

# Current Packet

## Just Finished

Step 1 - Refresh The Current Fixed-Formal First Bad Fact: reran the focused fixed-formal/aarch64 proof for idea 327. The current first bad fact is absent in this subset: the build was up to date and all four delegated tests passed, including `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication` and `c_testsuite_aarch64_backend_src_00204_c`.

## Suggested Next

Supervisor should decide whether idea 327 has enough evidence to close/deactivate or whether a broader fixed-formal probe is needed before lifecycle action.

## Watchouts

- No in-scope owner remains live from the delegated focused proof; do not implement under this idea unless fresh evidence shows an in-scope fixed-formal entry publication first bad fact.
- Treat prior HFA/floating, byval aggregate, local/value-home, `va_start`, aggregate helper, F128, scalar ALU, and frame/stack repairs as adjacent guardrails, not default scope.
- Do not weaken expectations, unsupported classifications, runner behavior, timeout policy, proof-log policy, or CTest registration.

## Proof

Ran the supervisor-selected proof exactly:

```sh
bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(backend_aarch64_instruction_dispatch|backend_aarch64_call_boundary_owner|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication|c_testsuite_aarch64_backend_src_00204_c)$"' > test_after.log 2>&1
```

Result: passed. `test_after.log` is the canonical proof log.
