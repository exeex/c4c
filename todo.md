Status: Active
Source Idea Path: ideas/open/358_rv64_object_route_abi_width_edges.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Route Simple Call and FPR ABI Edges

# Current Packet

## Just Finished

Completed Plan Step 4 producer-side RV64 FPR ABI edge repair for the
`riscv64-linux-gnu` representative path.

`target_profile_from_triple("riscv64-linux-gnu")` now resolves the backend ABI
to `RiscvLp64D`, matching the Linux GNU representative/sysroot hard-float
route. The existing prepared FPR formal-home identity contract now also runs
for `riscv64-linux-gnu` and proves `%p.a` publishes `fa0` plus the authoritative
FPR target register identity instead of a raw `a0` spelling.

## Suggested Next

Rerun `src/20030125-1.c` through the RV64 object-route representative command
to confirm the prepared dump now shows the FPR formal home as `fa0` and to
identify the next call/result boundary after this profile fix.

## Watchouts

- Treat `src/20030125-1.c` as a representative only. The semantic class is
  prepared FPR cast/call ABI lowering, not testcase-specific math folding.
- Admission is intentionally identity-backed for FPR homes. Do not add raw
  `register_name` fallback parsing for FPR spellings in object emission.
- This slice intentionally leaves explicit `riscv64-unknown-linux-gnu` soft
  profile behavior unchanged; only the `riscv64-linux-gnu` Linux GNU
  representative path was promoted to LP64D.

## Proof

```sh
cmake --build build --target c4cll backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_riscv_object_emission_test && ctest --test-dir build -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_riscv_object_emission)$' --output-on-failure > test_after.log
```

Result: passed. Proof log: `test_after.log`.
