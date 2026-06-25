Status: Active
Source Idea Path: ideas/open/358_rv64_object_route_abi_width_edges.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Route Simple Call and FPR ABI Edges

# Current Packet

## Just Finished

Completed Plan Step 4 prepared FPR ABI move-bundle destination resolution.

The RV64 prepared move-bundle emitter now treats a raw destination
`register_name` as a GPR hint and, when it does not resolve to a GPR, still
uses the prepared FPR ABI placement/identity to choose the target FPR. This
accepts prepared `before_return` ABI moves with `destination_register_name=fa0`
plus `destination_register_placement=fpr:call_result#0/w1` without adding raw
FPR-name parsing to object emission.

The focused object-emission regression now matches that real prepared shape for
the F32/F64 return move fixture and keeps raw `fa0` without a prepared placement
fail-closed with the shared unsupported move-bundle diagnostic.

## Suggested Next

Rerun the representative `src/20030125-1.c` RV64 backend progress proof to find
the next object-route boundary after the prepared FPR return ABI move and
placement-backed `fa0` destination resolution.

## Watchouts

- Treat `src/20030125-1.c` as a representative only. The semantic class is
  prepared FPR cast/call ABI lowering, not testcase-specific math folding.
- FPR move-bundle admission remains identity/placement-backed; raw FPR
  `register_name` parsing is still intentionally unsupported in object
  emission.
- The new terminator skip depends on the prepared before-return FPR ABI move
  lookup for the same block and source value. If the representative advances,
  inspect the next `prepared.dump` boundary rather than weakening this guard.
- The new destination fallback is deliberately gated on prepared FPR placement
  or value-home identity; keep raw ABI FPR text names rejected unless the
  preparer publishes target identity facts.

## Proof

```sh
cmake --build build --target c4cll backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_riscv_object_emission_test && ctest --test-dir build -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_riscv_object_emission)$' --output-on-failure > test_after.log
```

Result: passed. Proof log: `test_after.log`.
