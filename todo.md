Status: Active
Source Idea Path: ideas/open/690_call_abi_import_boundary_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Narrow Adapter-Owned Call ABI Helper Boundaries

# Current Packet

## Just Finished

Step 2 implementation packet for `plan.md` Step 2 completed. In
`src/backend/bir/lir_to_bir/call_abi.cpp`, extracted the AArch64 HFA aggregate
return semantic ABI decoration into one anonymous-namespace helper,
`lower_aarch64_hfa_aggregate_return_info`. The helper takes the target profile
and aggregate layout, detects AArch64 HFA return facts, computes the semantic
`CallResultAbiInfo`, preserves the existing `register_count` decoration, and
returns the corresponding `LoweredReturnInfo`. `lower_return_info_from_type`
now delegates that HFA aggregate-return branch to the helper without changing
the sret aggregate-return path.

## Suggested Next

Next coherent packet: continue Step 2 by looking for another narrow
adapter-owned signature, argument, return, byval, vararg, or HFA helper
boundary inside `call_abi.cpp` that does not require changing `calling.cpp`,
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
- If the next helper-boundary packet exposes a need to edit
  `calling.cpp`, prepared/prealloc files, target files, tests, or expectations,
  stop instead of broadening this packet.

## Proof

Supervisor-selected proof passed and wrote `test_after.log`:

```bash
cmake --build --preset default && ctest --test-dir build -R '^(frontend_lir_function_signature_type_ref|frontend_lir_call_type_ref|backend_codegen_route_aarch64_byval_global_payload_call_boundary|backend_codegen_route_aarch64_hfa_global_payload_call_boundary|backend_codegen_route_aarch64_hfa_global_payload_return|backend_codegen_route_riscv64_byval_aggregate_fixed_call|backend_codegen_route_riscv64_byval_formal_gpr_publication|backend_codegen_route_x86_64_byval_member_array_params_observe_semantic_bir|backend_codegen_route_x86_64_aggregate_param_return_pair_observe_semantic_bir)$' --output-on-failure | tee test_after.log
```

Result: build succeeded; focused CTest subset passed 9/9.
