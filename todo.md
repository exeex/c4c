Status: Active
Source Idea Path: ideas/open/690_call_abi_import_boundary_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish Call Import Evidence And First Packet Boundary

# Current Packet

## Just Finished

Step 1 evidence pass for `plan.md` Step 1 completed. Inspected
`ideas/open/690_call_abi_import_boundary_cleanup.md`,
`docs/lir_bir_adapter_boundary/ordered_followup_plan.md`,
`docs/lir_bir_adapter_boundary/interface_inventory_handoff.md`,
`docs/lir_bir_adapter_boundary/responsibility_classification_handoff.md`,
`docs/lir_bir_adapter_boundary/step1_adapter_boundary_inventory.md`,
`docs/lir_bir_adapter_boundary/step2_responsibility_classification.md`,
`src/backend/bir/lir_to_bir/call_abi.cpp`,
`src/backend/bir/lir_to_bir/calling.cpp`, and the relevant call declarations
in `src/backend/bir/lir_to_bir/lowering.hpp`.

## Suggested Next

First implementation packet: in `src/backend/bir/lir_to_bir/call_abi.cpp`,
extract the repeated semantic pointer-ABI decoration for sret/byval function
parameter import into one anonymous-namespace helper, then replace the local
inline lambdas in `lower_function_params_with_layouts` with that helper. Keep
the packet behavior-preserving: it should only centralize how imported BIR
`Param::abi` records set `size_bytes`, `align_bytes`, `sret_pointer`, and
`byval_copy` after `compute_call_arg_abi(target_profile, bir::TypeKind::Ptr)`.
Do not change ABI classification values, function signatures, BIR output, or
call lowering behavior.

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
  `compute_function_return_abi` in the first packet: `calling.cpp` and existing
  internal BIR handoff tests still reference that surface. Contracting those
  declarations would require a separate packet with explicit test ownership.
- If the sret/byval ABI helper extraction exposes a need to edit
  `calling.cpp`, prepared/prealloc files, target files, tests, or expectations,
  stop instead of broadening this packet.

## Proof

No build/test required or run for this evidence-only Step 1 todo update.
Recommended proof command for the selected first implementation packet:

```bash
cmake --build --preset default && ctest --test-dir build -R '^(frontend_lir_function_signature_type_ref|frontend_lir_call_type_ref|backend_codegen_route_aarch64_byval_global_payload_call_boundary|backend_codegen_route_aarch64_hfa_global_payload_call_boundary|backend_codegen_route_aarch64_hfa_global_payload_return|backend_codegen_route_riscv64_byval_aggregate_fixed_call|backend_codegen_route_riscv64_byval_formal_gpr_publication|backend_codegen_route_x86_64_byval_member_array_params_observe_semantic_bir|backend_codegen_route_x86_64_aggregate_param_return_pair_observe_semantic_bir)$' --output-on-failure
```

`test_after.log` was not produced because the delegated proof explicitly did
not require execution for this evidence-only packet.
