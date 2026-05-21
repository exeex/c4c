Status: Active
Source Idea Path: ideas/open/331_aarch64_variadic_stdarg_cursor_format_residual.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Format-Byte NUL-Test Materialization

# Current Packet

## Just Finished

Lifecycle switched from idea 326 to idea 331 after fresh `00204` evidence
reclassified the active first bad fact as AArch64 format-byte/NUL-test
materialization in `myprintf`, not an HFA register-save-area defect.

## Suggested Next

Execute Step 1: localize the generated-code boundary that stores the current
format byte with `strb`, reloads a wider value from the same stack address, and
lets `myprintf`'s `for (s = format; *s; s++)` loop overread past the
terminating NUL during the second stdarg `%7s %9s ...` invocation.

## Watchouts

Do not resume HFA register-save-area work until the format-byte overread is
repaired or disproven and the standalone HFA stdarg invocation is reached as
the true first bad fact.

Do not change implementation files, tests, expectations, runners, proof-log
policy, unsupported classifications, or CTest contracts in lifecycle packets.

## Proof

No code validation was run for this lifecycle-only switch. Existing proof
context remains in the supervisor-provided `todo.md` state after commit
`1f0917f5b` and the preserved root proof logs.
