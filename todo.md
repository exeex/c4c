Status: Active
Source Idea Path: ideas/open/358_rv64_object_route_abi_width_edges.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Route Simple Call and FPR ABI Edges

# Current Packet

## Just Finished

Completed Plan Step 4 prepared FPR immediate return lowering.

The RV64 prepared return emitter now accepts immediate F32/F64 return values
whose published bit pattern fits the current 12-bit immediate materializer. It
loads the bit pattern into `t0`, moves it to the hard-float return register with
`fmv.w.x fa0, t0` or `fmv.d.x fa0, t0`, then emits the normal return sequence.

The focused object-emission regression covers F32/F64 immediate return objects
and keeps unsupported shapes fail-closed: F128 immediate returns and F32/F64 bit
patterns outside the current materializable range still reject through the
shared unsupported terminator diagnostic.

## Suggested Next

Rerun the representative `src/20030125-1.c` RV64 backend progress proof to find
the next object-route boundary after prepared FPR immediate return lowering.

## Watchouts

- Treat `src/20030125-1.c` as a representative only. The semantic class is
  prepared FPR cast/call ABI lowering, not testcase-specific math folding.
- FPR move-bundle admission remains identity/placement-backed; raw FPR
  `register_name` parsing is still intentionally unsupported in object
  emission.
- Immediate FPR returns are deliberately bounded to bit patterns that fit the
  existing 12-bit `addi` materializer. Larger constants need a wider semantic
  integer materializer before this route should admit them.
- The representative previously included local `floor`/`sinf` stubs returning
  zero after `abort()`. This slice handles that as typed immediate return
  lowering, not by matching function names.

## Proof

```sh
cmake --build build --target c4cll backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_riscv_object_emission_test && ctest --test-dir build -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_riscv_object_emission)$' --output-on-failure > test_after.log
```

Result: passed. Proof log: `test_after.log`.
