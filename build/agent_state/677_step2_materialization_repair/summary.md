# 677 Step 2 Materialization Repair Summary

## Semantic Repair

Repaired the RV64 object-route consumer in
`src/backend/mir/riscv/codegen/object_emission.cpp` at
`fragment_for_prepared_call`.

For GPR call arguments whose source selection is
`PreparedCallArgumentSourceSelectionKind::LocalFrameAddressMaterialization`, the
object route now passes the ABI destination register directly to
`append_rv64_prepared_local_frame_address_call_argument_source`.

This matches the semantic contract already used by the text route: local frame
address materialization for a call argument is materialized into the outgoing
ABI argument register when that destination register is available. The repair is
not keyed to the focused test name, expected bytes, or a special-case source
register.

## Object Shape

Fresh object disassembly for the focused row now shows direct materialization:

```text
30: 02900313      li      t1, 0x29
34: 00612023      sw      t1, 0x0(sp)
38: 00028913      mv      s2, t0
3c: 00010513      mv      a0, sp
40: 00000097      auipc   ra, 0x0
                  R_RISCV_CALL_PLT read_local_address
44: 000080e7      jalr    ra
```

The previous two-step object route, `mv s1, sp; mv a0, s1`, is no longer emitted
for this local-frame-address call argument.

## Proof

Command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_cli_riscv64_call_arg_local_frame_address_materialization|backend_codegen_route_riscv64_call_arg_local_frame_address_materialization|backend_dump_riscv64_call_arg_local_frame_address_materialization'
```

Result: build succeeded and all three focused tests passed.

Canonical proof log: `test_after.log`.
