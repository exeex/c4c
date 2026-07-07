Status: Active
Source Idea Path: ideas/open/574_rv64_floating_point_binary_lowering.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Focused FP Binary Object-Emission Coverage

# Current Packet

## Just Finished

Completed Step 2: added focused backend object-emission coverage for scalar
RV64 floating-point binary lowering before implementation.

Added supported-behavior coverage for `bir.sdiv double` with FPR homes for both
operands and the result. The test expects the prepared object route to emit
`fdiv.d ft0, fa0, fa1` followed by `ret`, with no relocations.

Added fail-closed coverage for unsupported FP binary forms:

- `bir.sdiv fp128` remains rejected through the existing
  `unsupported_instruction_fragment` diagnostic.
- `bir.srem double` remains rejected through the existing
  `unsupported_instruction_fragment` diagnostic.

The focused test currently fails before implementation at the new supported
F64 binary assertion:

```text
expected prepared scalar fdiv.d RV64 object module to build, got `unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering; function=fp_binary; block=entry; block_index=0; instruction_index=0; instruction_kind=BinaryInst; owner=double %result`
```

## Suggested Next

Execute Step 3 from `plan.md`: implement RV64 object-route lowering for
supported scalar double FP binary operations, starting with `bir.sdiv double`
to `fdiv.d`, using prepared FPR operand/result homes and leaving unsupported FP
types/opcodes fail-closed.

## Watchouts

- The supported test is semantic and fixture-local; it does not reference
  `src/20000605-1.c`, `render_image_rgb_a`, `%t5`, or any representative-only
  spelling.
- BIR uses `BinaryOpcode::SDiv` for the observed floating division owner, so
  Step 3 must key lowering on both opcode and floating type instead of treating
  every `SDiv` as an integer division.
- The F128 and F64 remainder cases are deliberate fail-closed coverage and
  should not be weakened to make the test binary pass.

## Proof

Delegated proof command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'
```

Result: build passed; focused CTest failed as expected before implementation on
the new supported F64 binary object-emission assertion.

Proof log: `test_after.log`
