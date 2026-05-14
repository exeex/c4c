Status: Active
Source Idea Path: ideas/open/228_aarch64_module_phoenix_drafts_to_implementation.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Split Or Classify Compatibility Projection

# Current Packet

## Just Finished

Step 6 split the derived AArch64 compatibility projection out of
`src/backend/mir/aarch64/codegen/emit.cpp` into target-private
`src/backend/mir/aarch64/codegen/compatibility_projection.hpp` and
`src/backend/mir/aarch64/codegen/compatibility_projection.cpp`.

`emit.cpp` now stays in orchestration: validate handoff, build the MIR module,
derive compatibility-facing function records from canonical lowered MIR, and
assign the derived compatibility projection. The projection helper filters only
selected non-return machine nodes and does not act as semantic lowering
authority.

## Suggested Next

Next packet: supervisor or plan owner should decide whether Step 6 exhausts the
current runbook or whether another target-private codegen cleanup remains
inside the active idea before closure/deactivation.

## Watchouts

- Do not recreate deleted `module/function_traversal.cpp`,
  `module/operand_resolution.cpp`, `module/instruction_lowering.cpp`,
  `module/branch_control_lowering.cpp`, or `module/call_lowering.cpp`.
- `machine_printer.*` is still live code today; deleting it before the shared
  MIR printer route lands would break the public AArch64 asm path.
- Idea 224 owns replacing this target-local printer with common MIR traversal
  plus AArch64 target rendering hooks.
- The compatibility projection is now target-private codegen glue derived from
  canonical lowered MIR; do not add fallback lowering inputs there.
- `machine_printer.*`, `src/backend/mir/aarch64/module/*`, tests, `plan.md`,
  and `ideas/open/*` were not touched.

## Proof

`cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_module_skeleton|backend_aarch64_function_traversal|backend_aarch64_instruction_dispatch|backend_aarch64_operand_resolution|backend_aarch64_return_lowering|backend_aarch64_branch_control_lowering|backend_cli_aarch64_asm_external_return_zero_smoke|backend_cli_aarch64_asm_external_return_add_smoke|backend_cli_aarch64_asm_external_return_add_sub_chain_smoke'`

Passed. Proof log: `test_after.log`.
