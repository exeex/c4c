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

## Internal Unit Contract

Updated
`tests/backend/mir/backend_riscv_object_emission_test.cpp`,
`builds_prepared_local_frame_address_register_source_arg_call_object`, to
assert the repaired object-route contract directly. The fixture's
`LocalFrameAddressMaterialization` register argument now expects
`addi a0, sp, 24` immediately before the outgoing stack-argument setup, while
the second local-frame-address argument still checks the stack publication
sequence:

```text
addi a0, sp, 24
addi sp, sp, -8
addi s2, sp, 40
mv t3, s2
sd t3, 0(sp)
```

This preserves the stack-argument publication checks and removes the stale
expectation that the register argument must first publish through `s1`.

## Proof

Command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_cli_riscv64_call_arg_local_frame_address_materialization|backend_codegen_route_riscv64_call_arg_local_frame_address_materialization|backend_dump_riscv64_call_arg_local_frame_address_materialization|backend_riscv_object_emission'
```

Result: build succeeded; the three focused 677 rows and the nearby
`backend_riscv_object_emission` internal unit test passed.

Canonical proof log: `test_after.log`.
