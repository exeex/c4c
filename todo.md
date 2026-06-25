Status: Active
Source Idea Path: ideas/open/358_rv64_object_route_abi_width_edges.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Route Simple Call and FPR ABI Edges

# Current Packet

## Just Finished

Completed Plan Step 4 object-route admission for prepared RV64 hard-float
formal parameter homes.

The prepared-shape gate now admits formal register homes when either the
existing GPR helper accepts the home or `fpr_register_number_for_home` accepts
the published RV64 FPR target identity. The route still rejects a raw `fa0`
formal home when `PreparedTargetRegisterIdentity` is absent, so object emission
does not fall back to parsing FPR register-name strings.

Focused object-emission coverage now proves both edges:

- a prepared `float %p.a` formal home in `fa0` with RV64/FPR/Float identity
  builds a minimal object
- the same raw `fa0` spelling without target identity is rejected with the
  updated parameter-home diagnostic

## Suggested Next

Rerun `src/20030125-1.c` through the RV64 object route to expose the next real
instruction/call lowering boundary now that FPR formal parameter homes pass
prepared-shape admission.

## Watchouts

- Treat `src/20030125-1.c` as a representative only. The semantic class is
  prepared FPR cast/call ABI lowering, not testcase-specific math folding.
- Admission is intentionally identity-backed for FPR homes. Do not add raw
  `register_name` fallback parsing for FPR spellings in object emission.
- Do not broaden the cast type matrix. The representative still only needs
  `FPExt F32 -> F64` and already-covered `FPTrunc F64 -> F32`.
- Floating calls, libm behavior, and stack-slot FPR materialization remain
  outside this admission packet.

## Proof

```sh
cmake --build build --target backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_riscv_object_emission_test && ctest --test-dir build -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_riscv_object_emission)$' --output-on-failure > test_after.log
```

Result: passed. Proof log: `test_after.log`.
