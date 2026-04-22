# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 2.2.2
Current Step Title: Migrate Canonical Call Issuance, Cleanup, And Result Publication
Plan Review Counter: 1 / 6
# Current Packet

## Just Finished

Completed step 2.2.2 for the canonical result-transfer helper family by moving
the compiled ownership of `emit_call_move_f32_to_acc_impl`,
`emit_call_move_f64_to_acc_impl`, `emit_return_f32_to_reg_impl`,
`emit_return_f64_to_reg_impl`, `emit_return_f128_to_reg_impl`, and the
`emit_get_/emit_set_return_*_second_impl` helpers out of the lowering
translation units and into reviewed compiled owner
`core/x86_codegen_output.cpp`. Canonical call result publication and return
register/stack transfer now use the shared compiled output seam instead of
leaving those pure asm helpers owned inside mixed lowering files.

## Suggested Next

Stay on step 2.2.2 and migrate one remaining compiled call/return helper family
that still mixes policy with asm emission, likely `emit_call_cleanup_impl`,
`emit_call_store_result_impl`, or `emit_return_i128_to_regs_impl`, but keep the
packet bounded away from prepared-route admission and ABI classification.

## Watchouts

- The backend target still does not compile `shared_call_support.cpp` or
  `mod.cpp`; seam moves have to remain self-contained in compiled reviewed
  owners rather than re-linking dormant utilities.
- `call_lowering.cpp` and `return_lowering.cpp` still own the actual
  call/return policy flow; this packet moved only pure result-transfer asm
  helpers into `core/x86_codegen_output.cpp`.
- `shared_call_support.cpp` still carries dormant support definitions; do not
  treat this packet as permission to revive that file as an owner.
- Keep `call_lowering.cpp`, `memory.cpp`, and `mod.cpp` non-owning for this
  route; do not widen into ABI policy, prepared-route admission logic, or
  dormant utility reattachment.

## Proof

Step 2.2.2 canonical result-transfer helper migration on 2026-04-22:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed (`106/106`). Canonical log paths: `test_before.log`,
`test_after.log`
