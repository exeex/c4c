Status: Active
Source Idea Path: ideas/open/358_rv64_object_route_abi_width_edges.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Route Simple Call and FPR ABI Edges

# Current Packet

## Just Finished

Completed Plan Step 4 RV64 object-route FPR cast boundary repair for the
prepared formal-parameter path.

Prepared RV64 FPR values allocated into normal caller/callee-saved FPR homes now
publish structured target register identity from prepared placement metadata, so
the object route can lower `%t0 = bir.fpext float %p.a to double` when `%p.a`
is the formal `fa0` home and `%t0` is the prepared `ft0` FPR home. Focused
coverage now proves the prepared `%t0` identity and the object bytes for
`fcvt.d.s ft0, fa0, rne` without adding raw FPR register-name parsing or
broadening the supported cast opcode/type matrix.

## Suggested Next

Classify the next `src/20030125-1.c` RV64 object-route boundary after the FPR
cast repair. A supplemental rerun moved past `unsupported_floating_cast` and now
reports `unsupported_instruction_fragment: BIR instruction requires unsupported
RV64 object lowering`, likely at the following call/result boundary.

## Watchouts

- Treat `src/20030125-1.c` as a representative only. The semantic class is
  prepared FPR cast/call ABI lowering, not testcase-specific math folding.
- Admission is intentionally identity-backed for FPR homes. Do not add raw
  `register_name` fallback parsing for FPR spellings in object emission.
- The new producer identity is limited to known RV64 scalar FPR register
  placements (`ft0`, `fs1`, `fs2`) and fails closed for unknown placements.

## Proof

```sh
cmake --build build --target c4cll backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_riscv_object_emission_test && ctest --test-dir build -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_riscv_object_emission)$' --output-on-failure > test_after.log
```

Result: passed. Proof log: `test_after.log`.
