# 677 Step 1 Object/Text Divergence Summary

## Fresh Focused Proof

Command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_cli_riscv64_call_arg_local_frame_address_materialization'
```

Result: build succeeded, focused CTest failed as expected. Canonical proof log:
`test_after.log`.

Focused failure:

```text
Test #159: backend_cli_riscv64_call_arg_local_frame_address_materialization ...***Failed
[BACKEND_OBJ_MISSING_BYTES]
/workspaces/c4c/build/tests/backend/riscv64_call_arg_local_frame_address_materialization.o
did not contain 13050100
```

`13050100` is the little-endian encoding of the direct RV64
`addi a0, sp, 0` shape.

## Fresh Text Route Shape

Artifact: `build/agent_state/677_step1_object_text_divergence/focus_rv64.s`

The text route still materializes the local frame address directly into the ABI
argument register:

```text
main:
.Lmain_entry:
    addi sp, sp, -32
    sd ra, 24(sp)
    li t1, 41
    sw t1, 0(sp)
    mv s2, t0
    addi a0, sp, 0
    call read_local_address
```

The generated CTest text-route artifact at
`build/tests/backend/riscv64_call_arg_local_frame_address_materialization.s`
has the same `addi a0, sp, 0` line.

## Fresh Object Route Shape

Artifacts:

- `build/agent_state/677_step1_object_text_divergence/focus_rv64.o`
- `build/agent_state/677_step1_object_text_divergence/llvm_objdump.out`

Fresh object disassembly for `main` shows the two-step route:

```text
30: 02900313      li      t1, 0x29
34: 00612023      sw      t1, 0x0(sp)
38: 00028913      mv      s2, t0
3c: 00010493      mv      s1, sp
40: 00048513      mv      a0, s1
44: 00000097      auipc   ra, 0x0
                  R_RISCV_CALL_PLT read_local_address
48: 000080e7      jalr    ra
```

So the object route emits `addi s1, sp, 0` (disassembled as `mv s1, sp`),
then `mv a0, s1`, instead of directly emitting `addi a0, sp, 0`.

## Prepared Source-Selection Evidence

Artifact: `build/agent_state/677_step1_object_text_divergence/prepared_bir.out`

The focused prepared call argument is still explicitly selected as local frame
address materialization:

```text
arg index=0 value_bank=gpr source_encoding=register source_value_id=4 source_placement=gpr:callee_saved#0/w1 source_reg=s1 source_bank=gpr dest_placement=gpr:call_argument#0/w1 dest_reg=a0 dest_bank=gpr arg.source_selection=local_frame_address_materialization selection_source_value_id=4 selection_source_value=%lv.value selection_source_home=register selection_source_slot=#1 selection_source_stack_offset=0 selection_source_size=8 selection_source_align=8 selection_source_delta=0 selection_materialization_block=entry selection_materialization_inst=1 selection_materialization_slot=#1 selection_materialization_offset=0
```

The same prepared dump records the ordinary move bundle as
`reason=call_arg_register_to_register`, but the semantic source-selection
contract is the stronger `arg.source_selection=local_frame_address_materialization`.

## Prior 648 Evidence Reconfirmed

`build/agent_state/648_post656_call_evidence/summary.md` already recorded the
same contract boundary:

- representative call argument selected
  `arg.source_selection=local_frame_address_materialization`
- source home was a callee-saved GPR and ABI destination was `a0`
- representative object route emitted `mv s2, sp` followed by `mv a0, s2`
- focused text route emitted direct `addi a0, sp, 0`
- focused object route emitted `mv s1, sp` followed by `mv a0, s1`

The fresh Step 1 evidence matches that prior conclusion and is not tied to the
test name or expected-byte string alone.

## First Object/Text Boundary Hypothesis

Exact first divergent consumer:

`src/backend/mir/riscv/codegen/object_emission.cpp`,
`fragment_for_prepared_call`, in the
`PreparedCallArgumentSourceSelectionKind::LocalFrameAddressMaterialization`
branch around the current `argument.source_selection` handling.

Boundary detail:

- Text route: `src/backend/mir/riscv/codegen/prepared_call_emit.cpp`,
  `emit_riscv_simple_call`, computes
  `local_materialization_route` and emits `addi <destination_register_name>, sp,
  offset`. For the focus row, `destination_register_name` is the ABI argument
  register `a0`.
- Object route: `src/backend/mir/riscv/codegen/object_emission.cpp`,
  `fragment_for_prepared_call`, computes `source` from
  `argument.source_register_name` (`s1`) and then sets
  `publication_register = source.value_or(*destination)`. It passes that
  register into
  `append_rv64_prepared_local_frame_address_call_argument_source`, which encodes
  `addi publication_register, sp, offset`. Because `source` exists, the helper
  emits `addi s1, sp, 0`; the caller then appends `mv a0, s1`.

The helper
`append_rv64_prepared_local_frame_address_call_argument_source` itself appears
capable of encoding the direct shape when given `a0`; it is the object-route
caller in `fragment_for_prepared_call` that consumes
`LocalFrameAddressMaterialization` differently by preferring the prepared source
register over the ABI destination register.

No repair was implemented in this packet.
