# Current Packet

Status: Active
Source Idea Path: ideas/open/57_aarch64_variadic_va_arg_register_save_progression_repair.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Validate variadic route acceptance

## Just Finished

Completed Step 3 inline va_arg register-save publication repair for the
remaining AArch64 string/pointer path. Memory lowering now publishes va_list
cursor updates from the old cursor carrier, and edge publication treats
prepared va_list field loads with register homes as live carriers whose
dependency ordering is accounted for before clobbering the target register.

The generated `myprintf` register-save path now moves the old `gp_offset`
carrier before reloading the register-save base and no longer reloads
`[x21, #24]` after storing the advanced `gp_offset` for source address
formation.

## Suggested Next

After supervisor commit, route Step 4 to decide whether the variadic
register-save source idea is complete or needs a bounded follow-up.

## Watchouts

No test expectation changes were made. The proof depends on the coherent
`src/backend/mir/aarch64/codegen/memory.cpp` cursor-update lowering and
`src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp` live-carrier
publication changes; dispatch-only validation still leaves `00204` failing.

## Proof

Ran the delegated proof command and preserved output in `test_after.log`:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_pointer_select_aggregate_byte_copy|backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy|backend_codegen_route_aarch64_alu_unpublished_load_local_(after_call|call_boundary)|c_testsuite_aarch64_backend_src_00164_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00204_c)$'
```

Result: build passed and the focused subset passed `8/8`, including
`c_testsuite_aarch64_backend_src_00204_c`. Regression guard compared the prior
`7/8` baseline against this `8/8` proof and passed with no new failures. Proof
log path: `test_after.log`.
