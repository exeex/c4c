Status: Active
Source Idea Path: ideas/open/228_aarch64_module_phoenix_drafts_to_implementation.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Implement One Lowering Family At A Time

# Current Packet

## Just Finished

Step 5: Implement One Lowering Family At A Time moved AArch64 prepared function
traversal out of `module/function_traversal.cpp` and into new compiled
`codegen/traversal.cpp` with declarations in `codegen/traversal.hpp`.

`codegen/emit.cpp` now calls the codegen traversal API. `module/module.hpp` no
longer declares the traversal helpers, and the obsolete module translation unit
was deleted and replaced in backend CMake sources. Direct function-context tests
now include `codegen/traversal.hpp` and call the moved `aarch64_codegen`
helpers; expectations were unchanged.

## Suggested Next

Supervisor can review and commit this prepared-function traversal ownership
move, then choose the next bounded lowering family or route-review packet.

## Watchouts

- `codegen/traversal.hpp` still includes `module/module.hpp` because the moved
  API consumes module lowering contexts, MIR container aliases, and diagnostics;
  those shared module product/context structs were intentionally left in module
  ownership.
- Review checkpoint: the codegen headers introduced during the extraction
  remain target-private helper surfaces used by focused tests, not stable
  module public API.
- No lowering expectations or testcase contracts were changed in this packet.

## Proof

Ran:
`cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_function_traversal|backend_aarch64_instruction_dispatch|backend_aarch64_operand_resolution|backend_aarch64_return_lowering|backend_aarch64_branch_control_lowering|backend_cli_aarch64_asm_external_return_add_smoke|backend_cli_aarch64_asm_external_return_add_sub_chain_smoke'`

Result: passed; focused subset ran 7/7 tests after the build. Proof output is
preserved in `test_after.log`.
