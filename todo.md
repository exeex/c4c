Status: Active
Source Idea Path: ideas/open/228_aarch64_module_phoenix_drafts_to_implementation.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Implement One Lowering Family At A Time

# Current Packet

## Just Finished

Step 5: Implement One Lowering Family At A Time moved the implemented AArch64
branch/control terminator lowering family out of
`module/branch_control_lowering.cpp` and into compiled `codegen/comparison.cpp`,
with the active API declared by `codegen/comparison.hpp`.

`module/instruction_lowering.cpp` now dispatches branch and conditional-branch
terminators through the codegen comparison API, and
`src/backend/CMakeLists.txt` builds the new codegen owner instead of the old
module branch-control translation unit.

## Suggested Next

Supervisor can review this branch/control ownership move, then choose whether
the next packet should move another bounded lowering family out of the module
layer or continue splitting the remaining shared instruction surface into
domain-owned translation units.

## Watchouts

- `codegen/comparison.hpp` depends on `module/module.hpp` because the public
  branch/control API still consumes module lowering context and diagnostics;
  moving those shared context types would be a separate packet.
- `module/branch_control_lowering.cpp` has been removed from the active backend
  source list and deleted from the tree; remaining branch/control behavior is
  compiled from `codegen/comparison.cpp`.
- No tests or expectations were changed.

## Proof

Ran:
`cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_branch_control_lowering|backend_aarch64_instruction_dispatch|backend_cli_aarch64_asm_external_return_add_smoke|backend_cli_aarch64_asm_external_return_add_sub_chain_smoke'`

Result: passed; focused subset ran 4/4 tests after the build. Proof output is
preserved in `test_after.log`.
