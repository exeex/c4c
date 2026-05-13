Status: Active
Source Idea Path: ideas/open/211_aarch64_machine_instruction_node_contract.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Add Focused Proof

# Current Packet

## Just Finished

Step 5 of `plan.md` added fresh focused proof for the AArch64
machine-instruction-node contract and implemented record surfaces.

The delegated backend proof rebuilt the default preset and ran the full
`^backend_` CTest subset. It covered the AArch64 record/contract bucket,
including the target-instruction, target-record, prepared-scalar,
compare/branch, scalar, memory-operand, module-identity, operand-identity,
signature-metadata, frame-control, and data-identity tests.

No implementation files, test expectations, markdown roadmap files, `plan.md`,
or source idea files were changed in this proof-only packet.

## Suggested Next

Execute Step 6: Final Consistency Review. Compare the accepted diff against
`ideas/open/211_aarch64_machine_instruction_node_contract.md` and verify the
route does not preserve a `codegen -> asm text -> assembler parser` semantic
handoff.

## Watchouts

- `RecordSurfaceKind::RecordOnly` remains available only as a compatibility
  spelling for target MIR/pre-node records; new tests should prefer
  `TargetMirRecord`, `MachineInstructionNode`, `EncoderInput`, or
  `ExternalAssemblerInput`.
- `test_after.log` is now the canonical proof log for Step 5 and contains the
  exact build plus backend CTest output.
- Disabled MIR trace tests remain reported as disabled by CTest; the backend
  subset itself passed all run tests.
- Module display labels remain display/diagnostic fields, not lookup authority.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
passed. CTest reported `100% tests passed, 0 tests failed out of 131`; proof
log: `test_after.log`.
