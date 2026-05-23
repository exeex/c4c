Status: Active
Source Idea Path: ideas/open/352_aarch64_block_label_emission_ordering.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Refresh The Current `00176` Block-Emission First Bad Fact

# Current Packet

## Just Finished

Lifecycle activation created this refresh runbook from
`ideas/open/352_aarch64_block_label_emission_ordering.md`.

## Suggested Next

Execute Step 1: rebuild, run the supervisor-selected focused `00176` and
AArch64 branch/return/dispatch/call-boundary proof, then record whether any
current first bad fact still belongs to idea 352.

Suggested proof command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_branch_compare_records|backend_aarch64_compare_branch_candidate_records|backend_aarch64_branch_compare_contract|backend_aarch64_prepared_branch_records|backend_aarch64_branch_control_lowering|backend_aarch64_function_traversal|backend_aarch64_instruction_dispatch|backend_aarch64_call_boundary_owner|backend_aarch64_return_lowering|c_testsuite_aarch64_backend_src_00176_c)$'
```

## Watchouts

- Treat expectation, timeout, runner, unsupported-classification, CTest, and
  proof-log-policy changes as out of scope.
- Do not claim progress through a `00176`-specific, `partition`-specific,
  block-id-specific, label-suffix-specific, or emitted-instruction shortcut.
- Do not reopen recursive call preservation, local/formal frame-slot
  publication, indexed aggregate writeback, variadic/byval publication, scalar
  cast publication, or broad frame layout without fresh generated-code
  evidence and lifecycle routing.
- Preserve canonical proof-log names if producing executor proof logs:
  `test_before.log` and `test_after.log`.

## Proof

Not run during lifecycle activation.
