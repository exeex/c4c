Status: Active
Source Idea Path: ideas/open/690_call_abi_import_boundary_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Final Proof And Handoff

# Current Packet

## Just Finished

Step 5 final proof and handoff packet for `plan.md` Step 5 completed. Ran the
delegated acceptance proof for the completed call ABI import boundary cleanup
slice and refreshed `test_after.log`.

The final status-only packet did not edit implementation files, `plan.md`,
source ideas, tests, expectations, unsupported markers, allowlists,
prepared/prealloc files, target files, MIR files, runtime harness policy, or
root-level logs other than `test_after.log`.

The Step 4 audit result still stands: no downstream authority leak, named
testcase shortcut, expectation rewrite, unsupported downgrade, allowlist edit,
or weaker proof signal was found for the active cleanup range. No remaining
non-adapter follow-up was found during final handoff.

## Suggested Next

Next coherent packet: supervisor lifecycle review for whether the active
runbook should be closed, retired, or followed by a separate source idea. No
executor implementation packet is pending from this handoff.

## Watchouts

- Keep any lifecycle follow-up focused on call ABI import cleanup; do not absorb
  prepared call plans, physical register placement, outgoing stack layout,
  wrappers, helper protocols, MIR consumers, target emission, object/runtime
  behavior, tests, expectations, unsupported markers, allowlists, or harness
  policy.
- This final packet was proof/status only; implementation contracts were not
  changed here.
- `lir_to_bir_detail::compute_call_arg_abi` and
  `compute_function_return_abi` remain available to existing adapter callers;
  any future contraction of those declarations still needs explicit ownership.

## Proof

Supervisor-selected Step 5 final proof passed and wrote `test_after.log`:

```bash
cmake --build --preset default && ctest --test-dir build -R '^(frontend_lir_function_signature_type_ref|frontend_lir_call_type_ref|backend_codegen_route_aarch64_byval_global_payload_call_boundary|backend_codegen_route_aarch64_hfa_global_payload_call_boundary|backend_codegen_route_aarch64_hfa_global_payload_return|backend_codegen_route_riscv64_byval_aggregate_fixed_call|backend_codegen_route_riscv64_byval_formal_gpr_publication|backend_codegen_route_x86_64_byval_member_array_params_observe_semantic_bir|backend_codegen_route_x86_64_aggregate_param_return_pair_observe_semantic_bir)$' --output-on-failure | tee test_after.log
```

Result: build succeeded; focused CTest subset passed 9/9.
