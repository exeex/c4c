Status: Active
Source Idea Path: ideas/open/302_aarch64_scalar_machine_node_operand_forms.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Focused Operand Diagnostics

# Current Packet

## Just Finished

Lifecycle switched from umbrella idea 295 to focused idea 302 for AArch64
scalar machine-node operand forms. No implementation work has started under
this focused owner.

## Suggested Next

Delegate plan Step 1 to inspect focused operand diagnostics for
`c_testsuite_aarch64_backend_src_00064_c`,
`c_testsuite_aarch64_backend_src_00139_c`, and
`c_testsuite_aarch64_backend_src_00205_c`.

## Watchouts

- Keep `00104`, `00182`, `00140`, `00204`, `00216`, runtime buckets, and
  timeout/output-storm buckets out of this owner unless generated-code or
  diagnostic evidence proves shared scalar machine-node operand-form behavior.
- Do not change expectations, allowlists, unsupported classifications, timeout
  policy, runner behavior, or CTest registration.
- Do not claim testcase-count progress; prove semantic movement for the
  focused scalar operand-form family.

## Proof

Lifecycle-only switch. No tests were run and no proof logs were updated.
