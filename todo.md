Status: Active
Source Idea Path: ideas/open/228_aarch64_module_phoenix_drafts_to_implementation.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Implement One Lowering Family At A Time

# Current Packet

## Just Finished

Step 5: Implement One Lowering Family At A Time moved the implemented AArch64
return terminator lowering out of `module/instruction_lowering.cpp` and into
new compiled `codegen/returns.cpp` with its public dispatch API declared in
`codegen/returns.hpp`.

`module/instruction_lowering.cpp` now calls the codegen returns API for Return
terminators, while return record construction and return machine-instruction
construction live in the return-owned translation unit.

## Suggested Next

Supervisor can review and commit this return ownership move, then choose the
next bounded lowering family to extract from the module layer.

## Watchouts

- `codegen/alu.hpp` depends on `module/module.hpp` because the public scalar
  API still consumes module lowering context and diagnostics, and
  `codegen/returns.hpp` currently includes it to consume
  `BlockScalarLoweringState` for already-emitted scalar return values.
- No tests or expectations were changed.

## Proof

Ran:
`cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_return_lowering|backend_aarch64_instruction_dispatch|backend_cli_aarch64_asm_external_return_zero_smoke|backend_cli_aarch64_asm_external_return_add_smoke|backend_cli_aarch64_asm_external_return_add_sub_chain_smoke'`

Result: passed; focused subset ran 5/5 tests after the build. Proof output is
preserved in `test_after.log`.
