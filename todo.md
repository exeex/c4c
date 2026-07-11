Status: Active
Source Idea Path: ideas/open/690_call_abi_import_boundary_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Audit For Downstream Authority Leaks

# Current Packet

## Just Finished

Step 4 audit packet for `plan.md` Step 4 completed. Audited the active cleanup
range `4e892e96b^..HEAD`, covering the committed Step 2 and Step 3 slices:
`4e892e96b` (`Narrow semantic pointer param ABI decoration`), `7fb50b609`
(`Narrow HFA return ABI import helper`), and `f7e8195d6`
(`Narrow call argument metadata admission`).

Audit result: the committed cleanup stayed within adapter call ABI import and
call-lowering admission boundaries. The source diff is confined to
`src/backend/bir/lir_to_bir/call_abi.cpp` and
`src/backend/bir/lir_to_bir/calling.cpp`, with no implementation changes in
prepared/prealloc, target, MIR, runtime harness, tests, expectations,
unsupported markers, allowlists, or downstream target files.

Downstream authority check: prepared call plans, physical register placement,
outgoing stack layout, wrappers, helper protocols, aggregate transport/carrier
policy, and target emission stayed downstream. The only audited ABI metadata
movement was adapter-local helper extraction around semantic pointer ABI
decoration, AArch64 HFA return import facts, and call argument metadata
admission from structured LIR call args into BIR `CallArgAbiInfo`.

No new downstream follow-up was found during this audit.

## Suggested Next

Next coherent packet: proceed to `plan.md` Step 5 final proof and handoff if
the supervisor selects acceptance validation for the completed call ABI import
boundary cleanup slice.

## Watchouts

- Keep the final handoff focused on call ABI import cleanup; do not absorb
  prepared call plans, physical register placement, outgoing stack layout,
  wrappers, helper protocols, MIR consumers, target emission, object/runtime
  behavior, tests, expectations, unsupported markers, allowlists, or harness
  policy.
- The Step 4 audit found no named-testcase shortcut, expectation rewrite,
  unsupported downgrade, allowlist edit, or weaker proof signal in the active
  cleanup range.
- `lir_to_bir_detail::compute_call_arg_abi` and
  `compute_function_return_abi` remain available to existing adapter callers;
  any future contraction of those declarations still needs explicit ownership.

## Proof

Supervisor-selected Step 4 audit proof passed and wrote `test_after.log`:

```bash
cmake --build --preset default && ctest --test-dir build -R '^(frontend_lir_function_signature_type_ref|frontend_lir_call_type_ref|backend_codegen_route_aarch64_byval_global_payload_call_boundary|backend_codegen_route_aarch64_hfa_global_payload_call_boundary|backend_codegen_route_aarch64_hfa_global_payload_return|backend_codegen_route_riscv64_byval_aggregate_fixed_call|backend_codegen_route_riscv64_byval_formal_gpr_publication|backend_codegen_route_x86_64_byval_member_array_params_observe_semantic_bir|backend_codegen_route_x86_64_aggregate_param_return_pair_observe_semantic_bir)$' --output-on-failure | tee test_after.log
```

Result: build succeeded; focused CTest subset passed 9/9.
