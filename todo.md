Status: Active
Source Idea Path: ideas/open/613_abi_call_result_stack_frame_lowering.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Supported Frame Or Return Handling

# Current Packet

## Just Finished

Completed the Step 3 lifecycle handoff for the selected supported prepared
stack-frame rows. The missing dynamic-frame callee-saved GPR save-slot
placement authority is now split to
`ideas/open/626_prepared_dynamic_frame_callee_saved_slot_placement.md`, while
idea 613 remains active for ABI/RV64 consumer work that already has complete
prepared frame or return facts.

Focused `build/c4cll --codegen obj --target riscv64-linux-gnu -o /tmp/...`
probes for `src/20040811-1.c`, `src/pr43220.c`, and `src/vla-dealloc-1.c`
still stop at:

`unsupported_stack_frame: RV64 object route requires a supported prepared stack frame`

Prepared dumps confirm these rows publish frame size/alignment and dynamic-stack
operation metadata, but not enough concrete frame-save authority for the RV64
object-route stack-frame consumer:

- `src/20040811-1.c`: `frame_size=24`, `frame_alignment=8`,
  `has_dynamic_stack=yes`, `fixed_slots_use_fp=yes`,
  `requires_stack_save_restore=yes`, with `stack_save`, `dynamic_alloca`, and
  `stack_restore` operations.
- `src/pr43220.c`: `frame_size=40`, `frame_alignment=8`,
  `has_dynamic_stack=yes`, `fixed_slots_use_fp=yes`,
  `requires_stack_save_restore=yes`, with two `dynamic_alloca` operations and a
  `stack_restore`.
- `src/vla-dealloc-1.c`: `frame_size=24`, `frame_alignment=8`,
  `has_dynamic_stack=yes`, `fixed_slots_use_fp=yes`,
  `requires_stack_save_restore=yes`, with one `dynamic_alloca` and multiple
  `stack_restore` operations.

The attempted admission analysis showed the current RV64 frame validator has
two distinct blockers, both real prepared-authority gaps for this packet:

- Dynamic/fixed frame rows set `has_dynamic_stack=yes` and
  `uses_frame_pointer_for_fixed_slots=yes`, while the object route currently
  only validates fixed `sp`-relative prepared frames.
- Their `saved_register` rows name callee-saved GPRs such as `s1` and `s2`,
  but do not publish concrete `slot_placement` save-slot offsets/sizes for the
  function prologue/epilogue. Inferring those save locations from final frame
  size, register order, or testcase shape would violate the packet boundary.

Because the selected rows lack explicit prepared callee-saved save-slot
placements, they are no longer Step 3 implementation positives for idea 613.
The correct next move inside idea 613 is a focused residual refresh for stack
frame or prepared return rows that already publish complete authority.

## Suggested Next

Refresh Step 3 residuals for complete prepared frame/return facts. Keep
`src/20040811-1.c`, `src/pr43220.c`, and `src/vla-dealloc-1.c` under idea 626
unless their prepared dumps now publish explicit callee-saved GPR save-slot
placements. If no complete-authority Step 3 breadth remains, advance to Step 4
for adjacent ABI consumer authority or Step 5 close-readiness classification,
backed by the backend subset proof command.

## Watchouts

- Keep `src/20000808-1.c` under idea 624 and `src/20020529-1.c` under idea
  625; do not reclassify missing prepared authority as RV64 consumer progress.
- Keep dynamic/fixed frame callee-saved save-slot placement production under
  idea 626; do not infer save slots in RV64 object emission.
- Do not infer return destination home authority for `src/20001130-2.c` or
  `src/20080719-1.c`; their before-return rows remain producer-authority gaps.
- Do not expand the next frame packet into FPR save/restore, generic
  move-bundle authority production, local/global producer repair,
  runtime/library/variadic policy, or named-case-only frame handling.
- Preserve Step 2 positive guards: `src/20000603-1.c`, `src/20021219-1.c`,
  and `src/pr77767.c`.
- `src/20020314-1.c` remains a compatibility watchout with FPR-heavy
  frame/call details; do not fold FPR dynamic-frame handling into the first GPR
  dynamic-frame packet.

## Proof

Ran exactly:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Result: passed, `346/346` backend tests; `test_after.log` reports
`100% tests passed, 0 tests failed out of 346` and is the preserved proof log.
