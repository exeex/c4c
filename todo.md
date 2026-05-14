Status: Active
Source Idea Path: ideas/open/228_aarch64_module_phoenix_drafts_to_implementation.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Implement One Lowering Family At A Time

# Current Packet

## Just Finished

Step 5: Implement One Lowering Family At A Time moved the AArch64 prepared
block dispatch scaffold out of `module/instruction_lowering.cpp` and into new
compiled `codegen/dispatch.cpp` with public declarations in
`codegen/dispatch.hpp`.

`module/function_traversal.cpp` now calls the codegen dispatch API, and
`module/module.hpp` no longer declares the dispatch helpers or dispatch result
type. The obsolete `module/instruction_lowering.cpp` translation unit was
deleted and removed from backend CMake sources.

## Suggested Next

Supervisor can review and commit this dispatch ownership move, then choose the
next bounded lowering family or route-review packet.

## Watchouts

- `codegen/alu.hpp` depends on `module/module.hpp` because the public scalar
  API still consumes module lowering context and diagnostics, and
  `codegen/returns.hpp` currently includes it to consume
  `BlockScalarLoweringState` for already-emitted scalar return values.
- Direct dispatch tests needed a narrow public-API update to include
  `codegen/dispatch.hpp` and call the moved `aarch64_codegen` helpers; no test
  expectations were changed.

## Proof

Ran:
`cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_instruction_dispatch|backend_aarch64_function_traversal|backend_aarch64_return_lowering|backend_aarch64_branch_control_lowering|backend_cli_aarch64_asm_external_return_add_smoke|backend_cli_aarch64_asm_external_return_add_sub_chain_smoke'`

Result: passed; focused subset ran 6/6 tests after the build. Proof output is
preserved in `test_after.log`.
