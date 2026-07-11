Status: Active
Source Idea Path: ideas/open/690_call_abi_import_boundary_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Narrow Call-Lowering Admission Boundaries

# Current Packet

## Just Finished

Step 3 implementation packet for `plan.md` Step 3 completed. In
`src/backend/bir/lir_to_bir/calling.cpp`, extracted the local
`apply_call_arg_metadata` lambda from `lower_call_inst` into an
anonymous-namespace helper. The helper takes the structured call args,
argument index, and `bir::CallArgAbiInfo`, then applies
`aarch64_hfa_lane_count`, `aarch64_hfa_lane_index`, and
`aarch64_stack_align_bytes` exactly as the local lambda did. `lower_call_inst`
now passes `call.structured_args` at each existing metadata application site
without changing call admission behavior.

## Suggested Next

Next coherent packet: continue Step 3 with another narrow call-lowering
admission-boundary cleanup inside `calling.cpp` only, if the supervisor
selects one that does not require changing call ABI import helpers,
prepared/prealloc files, target files, MIR files, tests, expectations,
unsupported markers, allowlists, or runtime harness policy.

## Watchouts

- Keep this idea confined to call ABI import cleanup.
- Evidence boundary: `call_abi.cpp` owns signature, return, byval, vararg,
  HFA, and semantic `CallArgAbiInfo`/`CallResultAbiInfo` import; `calling.cpp`
  owns direct/indirect call admission, inline asm metadata, runtime/intrinsic
  call admission, call argument source relationships, and AArch64 variadic HFA
  carrier expansion.
- Excluded downstream ownership remains prepared call plans,
  `PreparedBirModule`, physical register placement, outgoing stack layout,
  aggregate transport lanes, wrappers, helper protocols, MIR consumers, target
  emission, object/runtime behavior, tests, expectations, unsupported markers,
  allowlists, and harness policy.
- Do not remove or rename `lir_to_bir_detail::compute_call_arg_abi` or
  `compute_function_return_abi`: `calling.cpp` and existing
  internal BIR handoff tests still reference that surface. Contracting those
  declarations would require a separate packet with explicit test ownership.
- If the next helper-boundary packet exposes a need to edit call ABI import
  helpers, prepared/prealloc files, target files, MIR files, tests, or
  expectations, stop instead of broadening that packet.
- For Step 3 cleanup, keep helper extraction behavior-preserving. Do not turn
  admission cleanup into expectation changes, unsupported markers, or
  testcase-shaped shortcuts.

## Proof

Supervisor-selected proof passed and wrote `test_after.log`:

```bash
cmake --build --preset default && ctest --test-dir build -R '^(frontend_lir_function_signature_type_ref|frontend_lir_call_type_ref|backend_codegen_route_aarch64_byval_global_payload_call_boundary|backend_codegen_route_aarch64_hfa_global_payload_call_boundary|backend_codegen_route_aarch64_hfa_global_payload_return|backend_codegen_route_riscv64_byval_aggregate_fixed_call|backend_codegen_route_riscv64_byval_formal_gpr_publication|backend_codegen_route_x86_64_byval_member_array_params_observe_semantic_bir|backend_codegen_route_x86_64_aggregate_param_return_pair_observe_semantic_bir)$' --output-on-failure | tee test_after.log
```

Result: build succeeded; focused CTest subset passed 9/9.
