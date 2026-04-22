# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 2.2.3
Current Step Title: Migrate Canonical Return-Lane Handoff And Audit Remaining Holdouts
Plan Review Counter: 0 / 6
# Current Packet

## Just Finished

Completed step 2.2.3 for the remaining canonical return-lane handoff helpers by
moving `emit_return_i128_to_regs_impl` and the adjacent
`emit_return_int_to_reg_impl` out of `lowering/return_lowering.cpp` and into
reviewed compiled owner `core/x86_codegen_output.cpp`. `return_lowering.cpp`
now keeps return value policy flow plus epilogue sequencing, while the pure
register-handoff asm helpers live with the other reviewed return publication
helpers behind the compiled output seam.

## Suggested Next

Audit whether any non-policy return-lane holdouts remain outside reviewed
owners after this seam move; if not, have the supervisor decide whether step
2.2.3 is exhausted or whether a narrower follow-on packet is still needed for a
different mixed-owner family.

## Watchouts

- The backend target still does not compile `shared_call_support.cpp` or
  `mod.cpp`; seam moves have to remain self-contained in compiled reviewed
  owners rather than re-linking dormant utilities.
- `return_lowering.cpp` still owns return-path policy flow, including the
  F128-special handling and epilogue sequencing; this packet moved only pure
  register-handoff asm helpers into `core/x86_codegen_output.cpp`.
- `shared_call_support.cpp` still carries dormant support definitions; do not
  treat this packet as permission to revive that file as an owner.
- Keep `call_lowering.cpp`, `memory.cpp`, and `mod.cpp` non-owning for this
  route; do not widen into ABI policy, epilogue policy, prepared-route logic,
  or dormant utility reattachment.

## Proof

Step 2.2.3 canonical return-lane handoff helper migration on 2026-04-22:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed (`106/106`). Canonical log paths: `test_before.log`,
`test_after.log`
