Status: Active
Source Idea Path: ideas/open/613_abi_call_result_stack_frame_lowering.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Supported Frame Or Return Handling

# Current Packet

## Just Finished

Completed Step 3, "Add Supported Frame Or Return Handling", as a blocker
classification packet after inspecting the selected supported prepared
stack-frame rows. No implementation files, focused backend tests, plan files,
idea files, expectations, unsupported markers, allowlists, or timeout/accounting
files were changed.

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
placements, this packet did not add RV64 lowering. The correct next move is a
producer-authority split or plan-owner rewrite for prepared dynamic-frame
save-slot publication before the RV64 object consumer can lower these rows
semantically.

## Suggested Next

Split the missing prepared dynamic-frame callee-saved save-slot authority into
a producer-owned follow-up, then refresh Step 3 residuals. The follow-up should
publish explicit save-slot placements for dynamic/fixed-frame callee-saved GPR
rows before RV64 object emission consumes `has_dynamic_stack=yes` /
`fixed_slots_use_fp=yes` frame plans.

## Watchouts

- Keep `src/20000808-1.c` under idea 624 and `src/20020529-1.c` under idea
  625; do not reclassify missing prepared authority as RV64 consumer progress.
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
