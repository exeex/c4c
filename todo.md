Status: Active
Source Idea Path: ideas/open/228_aarch64_module_phoenix_drafts_to_implementation.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Implement One Lowering Family At A Time

# Current Packet

## Just Finished

Step 5: Implement One Lowering Family At A Time moved AArch64 operand
resolution out of `module/operand_resolution.cpp` and into new compiled
`codegen/operands.cpp` with public declarations and operand-resolution types in
`codegen/operands.hpp`.

`codegen/alu.cpp`, `codegen/comparison.cpp`, and `codegen/returns.cpp` now call
the codegen operand API. `module/module.hpp` no longer declares or owns
operand-resolution helpers/types, and the obsolete module translation unit was
deleted and removed from backend CMake sources.

## Suggested Next

Supervisor can review and commit this operand-resolution ownership move, then
choose the next bounded lowering family or route-review packet.

## Watchouts

- `codegen/operands.hpp` still includes `module/module.hpp` because the moved
  API consumes module lowering contexts and diagnostics; those shared context
  and diagnostic structs were intentionally left in module ownership.
- Review checkpoint: `codegen/*.hpp` headers introduced during the extraction
  are target-private implementation surfaces used by focused tests, not stable
  module public API.
- Direct operand-resolution tests needed a narrow public-API update to include
  `codegen/operands.hpp` and call the moved `aarch64_codegen` helpers; no test
  expectations were changed.

## Proof

Ran:
`cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_operand_resolution|backend_aarch64_instruction_dispatch|backend_aarch64_return_lowering|backend_aarch64_branch_control_lowering|backend_cli_aarch64_asm_external_return_add_smoke|backend_cli_aarch64_asm_external_return_add_sub_chain_smoke'`

Result: passed; focused subset ran 6/6 tests after the build. Proof output is
preserved in `test_after.log`.
