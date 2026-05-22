Status: Active
Source Idea Path: ideas/open/329_aarch64_variadic_aggregate_va_arg_call_setup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh The Current Post-Va-Arg Call Setup First Bad Fact

# Current Packet

## Just Finished

Step 1 - Refresh The Current Post-Va-Arg Call Setup First Bad Fact: refreshed the current post-aggregate-`va_arg` ordinary call setup first bad fact. The supervisor-selected focused proof passed for call setup coverage and the `00204.c` representative, so the current first bad fact is classified as absent. No generated-artifact inspection was needed because the focused proof did not fail.

## Suggested Next

Proceed to Step 2 lifecycle handling: with no live in-scope first bad fact, request plan-owner closure or the supervisor-approved closure path using the current focused proof result.

## Watchouts

- Do not implement under this idea unless new generated-code evidence shows an in-scope post-aggregate-`va_arg` ordinary call setup first bad fact.
- The refreshed proof gives no current evidence to reopen byval aggregate lane publication, non-HFA aggregate materialization, HFA/floating, fixed-formal, local/value-home, frame/formal, or global initializer work from this idea.
- Do not weaken expectations, unsupported classifications, runner behavior, timeout policy, proof-log policy, or CTest registration.

## Proof

Passed:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(backend_aarch64_instruction_dispatch|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication|c_testsuite_aarch64_backend_src_00204_c)$"' > test_after.log 2>&1`

Proof log: `test_after.log`. The proof was sufficient for this classification packet because all three selected tests passed, including post-`va_arg` prepared-BIR publication coverage and the `00204.c` representative.
