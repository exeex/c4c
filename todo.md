Status: Active
Source Idea Path: ideas/open/358_rv64_object_route_abi_width_edges.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Route Simple Call and FPR ABI Edges

# Current Packet

## Just Finished

Completed Plan Step 4 producer-side RV64 FPR ABI identity publication for fixed
formal register homes.

`target_register_profile` now exposes an ABI-placement-to-target-register
identity helper for RV64 call argument/result slots, mapping ABI slot 0 to the
physical `a0`/`fa0` register index 10 rather than treating ABI slot numbers as
physical indices. `value_homes.cpp` uses that helper when publishing fixed
formal ABI register homes, so RV64 hard-float formals passed in `fa*` registers
carry `PreparedTargetRegisterIdentity` alongside their diagnostic register
spelling.

Focused coverage in `backend_prepare_frame_stack_call_contract_test` proves a
hard-float RV64 `F32` formal home is `register fa0` with
`target_arch=Riscv64`, `bank=Fpr`, `register_class=Float`, and
`physical_index=10`. No object-emission raw register-name parsing was added.

## Suggested Next

Re-run the representative object route for `src/20030125-1.c` and classify the
next first failing instruction now that RV64 FPR formal parameter homes publish
authoritative target register identity.

## Watchouts

- Treat `src/20030125-1.c` as a representative only. The semantic class is
  prepared FPR cast/call ABI lowering, not testcase-specific math folding.
- The producer-side formal home contract is now in place for hard-float RV64
  ABI registers. If the same cast still fails, inspect whether the prepared
  module is using a hard-float target profile and whether the source home has
  the new identity before changing object emission.
- Do not broaden the cast type matrix. The representative still only needs
  `FPExt F32 -> F64` and already-covered `FPTrunc F64 -> F32`.
- Preserve the object route's reliance on prepared register facts. Do not add
  fallback parsing of ABI register spellings like `fa0` in object emission.
- Floating calls, libm behavior, and stack-slot FPR materialization remain
  outside this classification packet.

## Proof

Delegated proof passed:

```sh
cmake --build build --target backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_riscv_object_emission_test && ctest --test-dir build -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_riscv_object_emission)$' --output-on-failure > test_after.log
```

Proof logs:
- `test_after.log`
