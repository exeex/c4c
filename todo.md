Status: Active
Source Idea Path: ideas/open/626_prepared_dynamic_frame_callee_saved_slot_placement.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Callee-Saved Placement Evidence

# Current Packet

## Just Finished

Step 1 refreshed prepared-frame and RV64 object-route evidence for the target
rows:

| Row | Current prepared facts | Missing callee-saved GPR placement facts | RV64 object-route bucket |
| --- | --- | --- | --- |
| `src/20040811-1.c` | `@main stable_base=fp frame_size=24 frame_alignment=8 has_dynamic_stack=yes`; dynamic ops are `stack_save`, one `dynamic_alloca`, and `stack_restore`; saved GPR identities are `s1 save_index=0` and `s2 save_index=1`; call preservation records route `%t0` through `callee_saved_register:s1`. | `saved_register` rows publish register identity, save index, width, and occupied units, but no `slot_placement` payload with slot id, stack offset, size, alignment, or fixed-location authority. | `unsupported_stack_frame: RV64 object route requires a supported prepared stack frame` during frame-size admission, because saved callee-saved GPRs lack complete prepared save-slot placement facts. |
| `src/pr43220.c` | `@main stable_base=fp frame_size=40 frame_alignment=8 has_dynamic_stack=yes`; dynamic ops are `stack_save`, two `dynamic_alloca`s, and `stack_restore`; saved GPR identities are `s1 save_index=0` and `s2 save_index=1`; call preservation records route `%t0` through `callee_saved_register:s1` across both dynamic allocas. | Same missing `slot_placement` authority for `s1` and `s2`: no producer-owned slot id, stack offset, size, alignment, or fixed-location facts. | Same `unsupported_stack_frame: RV64 object route requires a supported prepared stack frame` admission bucket. |
| `src/vla-dealloc-1.c` | `@main stable_base=fp frame_size=24 frame_alignment=8 has_dynamic_stack=yes`; dynamic ops are `stack_save`, one `dynamic_alloca`, and two `stack_restore`s; saved GPR identities are `s1 save_index=0` and `s2 save_index=1`; call preservation records route `%t0` through `callee_saved_register:s1`. | Same missing `slot_placement` authority for `s1` and `s2`: no producer-owned slot id, stack offset, size, alignment, or fixed-location facts. | Same `unsupported_stack_frame: RV64 object route requires a supported prepared stack frame` admission bucket. |

Evidence commands used for the rows:

- `./build/c4cll --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/<row>`
- `./build/c4cll --codegen obj --target riscv64-linux-gnu tests/c/external/gcc_torture/src/<row> -o /tmp/c4c626_evidence/<row>.o`

The current exact residual owner for this idea is prepared frame-plan
production: it must publish complete `PreparedSavedRegisterSlotPlacement`
facts for saved GPRs. RV64 already rejects incomplete prepared saved-register
slot authority through prepared stack-frame admission. Residual buckets outside
this Step 1 evidence are dynamic-stack object lowering, local/global memory
repair, move-bundle authority, runtime/library policy, and expectation changes.

## Suggested Next

Run Step 2 producer-authority tracing in `src/backend/prealloc/`: locate where
saved callee-saved GPR metadata is created and identify the narrow frame-layout
authority that can attach complete slot placement facts without RV64 inference.

## Watchouts

- Do not infer callee-saved save-slot placements in RV64 from frame size,
  register order, source filename, or final assembly shape.
- Keep FPR placement, move-bundle authority, local/global memory repair, call
  policy, and expectation changes outside this idea.
- Preserve dynamic stack operation metadata and existing frame size/alignment
  facts while adding placement authority.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
passed. Proof log: `test_after.log`.
