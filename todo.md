# Current Packet

Status: Active
Source Idea Path: ideas/open/57_aarch64_variadic_va_arg_register_save_progression_repair.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Validate variadic route acceptance

## Just Finished

Classified Step 3 acceptance after the aggregate register-save ordering fix.
The remaining `00204` stdarg string corruption is outside this packet's owned
`src/backend/mir/aarch64/codegen/variadic.cpp` route.

The failing string-aggregate cases in `myprintf` are already expanded in
prepared BIR as inline `vaarg.regtry`/`vaarg.reg`/`vaarg.stack` CFG, not as
`MachineOpcode::VariadicVaArgScalar` printed by `variadic.cpp`. The prepared
BIR for the first string case computes `%t20 = %lv.ap.8 + sext(%t10)`, where
`%t10` is the old `gp_offset`, but the generated AArch64 register-save path
loads `[x21, #24]` again after storing the advanced `gp_offset` and therefore
uses the post-increment offset as the source address. That is the same semantic
ordering fault, but its owner is the lowered inline va_arg/edge-publication
route for string aggregates, not the variadic scalar helper printer owned by
this packet.

## Suggested Next

Delegate a packet owning the inline va_arg lowering/publication path that turns
the prepared BIR string-aggregate `vaarg.reg` source into AArch64, with focus on
preserving the old `gp_offset` carrier across the store of the advanced
`gp_offset`.

## Watchouts

Do not rework aggregate HFA helper ordering for this symptom: generated HFA
aggregate helper paths now use `ldrsw` old-offset address formation before
storing the advanced `fp_offset`. For the string case, the bad assembly appears
around `myprintf` block `.LBB154_19`: it stores the advanced `[x21, #24]`, then
reloads `[x21, #24]` for the register-save source address. The BIR source for
that case expected the old `%t10` offset.

Treat existing dirty `src/backend/mir/aarch64/codegen/memory.cpp`,
`src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`, and transient
`review/*` files as external context.

## Proof

Ran the delegated proof command and preserved output in `test_after.log`:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_pointer_select_aggregate_byte_copy|backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy|backend_codegen_route_aarch64_alu_unpublished_load_local_(after_call|call_boundary)|c_testsuite_aarch64_backend_src_00164_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00204_c)$'
```

Result: build passed (`ninja: no work to do`), focused subset `7/8` passed, and
only `c_testsuite_aarch64_backend_src_00204_c` failed with
`[RUNTIME_MISMATCH]`. This proof is sufficient for classification but blocked
for acceptance because the remaining owner is outside the packet's owned files.
Proof log path: `test_after.log`.
