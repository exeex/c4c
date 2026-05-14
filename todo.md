Status: Active
Source Idea Path: ideas/open/228_aarch64_module_phoenix_drafts_to_implementation.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Implement One Lowering Family At A Time

# Current Packet

## Just Finished

Step 5: Implement One Lowering Family At A Time moved the AArch64 prepared
module build/projection implementation out of `module/module.cpp` and into
compiled `codegen/emit.cpp`, with the active API declared by `codegen/emit.hpp`.
`module::build` is now a thin delegation wrapper around `codegen::build_module`.

The slice keeps the existing module data types and lowering-family helpers in
place, derives compatibility machine nodes from the canonical MIR functions in
the new codegen translation unit, and removes the stale legacy text-emitter
declarations from the active AArch64 `emit.hpp`.

## Suggested Next

Supervisor can review this module-build boundary move, then choose whether the
next packet should move another bounded lowering family out of the module layer
or continue splitting the remaining shared instruction surface into domain-owned
translation units.

## Watchouts

- `module/module.hpp` still declares the module data types and the current
  lowering helper APIs used by traversal, branch, return, and operand tests;
  moving those declarations would be a separate packet.
- `codegen/emit.hpp` now depends on `module/module.hpp` because the public
  prepared-module product type remains module-owned.
- No branch/scalar/memory lowering behavior was moved or changed in this slice.

## Proof

Ran:
`cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_module_skeleton|backend_aarch64_function_traversal|backend_aarch64_instruction_dispatch|backend_aarch64_return_lowering|backend_aarch64_branch_control_lowering|backend_cli_aarch64_asm_external_return_zero_smoke|backend_cli_aarch64_asm_external_return_add_smoke|backend_cli_aarch64_asm_external_return_add_sub_chain_smoke'`

Result: passed; focused subset ran 8/8 tests after the build. Proof output is
preserved in `test_after.log`.
