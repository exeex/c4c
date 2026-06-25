Status: Active
Source Idea Path: ideas/open/348_vrm_regalloc_mir_and_rv64_assembler_bridge.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: RV64 Inline Asm MIR Operand Substitution

# Current Packet

## Just Finished

Completed Step 4: RV64 Inline Asm MIR Operand Substitution. RV64 inline-asm
constraint classification and prepared operand substitution now accept the full
`VR`/`VRM1`/`VRM2`/`VRM4`/`VRM8` vector-register family. Focused object-route
coverage proves grouped vector operands substitute only their base `vN`
register, tied `VRM8` operands reuse the same base register, scalar/vector
mixed operands keep their own homes, and scalar `.insn r` object-route
expectations remain closed to vector homes.

## Suggested Next

Delegate the next coherent RV64 bridge packet: connect the prepared substitution
and `.insn.d` shape facts to the next MIR/object emission handoff without
weakening scalar `.insn r` encoder constraints or adding VRM function-call ABI
behavior.

## Watchouts

- Do not add any VRM function-call ABI behavior.
- Do not downgrade supported-path expectations to unsupported.
- The `.insn r` object encoder remains intentionally GPR-only; vector support in
  this slice is prepared template substitution and `.insn.d` carrier handling.
- Scalar integer values still fail vector-register carrier compatibility through
  the existing `backend_prepare_liveness` negative fixture.
- Grouped vector substitution deliberately uses the assigned base register name,
  not occupied-register list rendering.

## Proof

Passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_object_emission|backend_prepare_liveness)$'`

Proof log: `test_after.log`.
