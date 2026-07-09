Status: Active
Source Idea Path: ideas/open/626_prepared_dynamic_frame_callee_saved_slot_placement.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Breadth And Close-Readiness

# Current Packet

## Just Finished

Step 5 re-ran the idea-626 representative rows through prepared dumps and the
RV64 object route after the producer and RV64 consumer guard slices.

Evidence commands:

- `./build/c4cll -I tests/c/external/gcc_torture --target riscv64-linux-gnu --dump-prepared-bir <row>`
- `./build/c4cll -I tests/c/external/gcc_torture --codegen obj --target riscv64-linux-gnu <row> -o /tmp/c4c626_step5_evidence/<row>.o`

Row classifications:

- `tests/c/external/gcc_torture/src/20040811-1.c`: prepared dump now publishes
  complete saved GPR placement for `s1` and `s2`
  (`slot#4+stack24`, `slot#5+stack32`, size/alignment 8, fixed location). The
  RV64 object route still rejects with `unsupported_stack_frame: RV64 object
  route requires a supported prepared stack frame`. Residual owner: dynamic
  local-memory/fixed-slot object lowering, because the row has
  `has_dynamic_stack=yes`, `fixed_slots_use_fp=yes`, ordinary frame-slot
  accesses, and pointer-value local-memory accesses.
- `tests/c/external/gcc_torture/src/pr43220.c`: prepared dump now publishes
  complete saved GPR placement for `s1` and `s2`
  (`slot#7+stack40`, `slot#8+stack48`, size/alignment 8, fixed location). The
  RV64 object route still rejects with the same supported-stack-frame
  diagnostic. Residual owner: dynamic local-memory/fixed-slot object lowering;
  this row has two dynamic allocas plus ordinary frame-slot and pointer-value
  accesses.
- `tests/c/external/gcc_torture/src/vla-dealloc-1.c`: prepared dump now
  publishes complete saved GPR placement for `s1` and `s2`
  (`slot#4+stack24`, `slot#5+stack32`, size/alignment 8, fixed location). The
  RV64 object route still rejects with the same supported-stack-frame
  diagnostic. Residual owner: dynamic local-memory/fixed-slot object lowering;
  this row has dynamic stack save/alloca/restore operations, fixed slots using
  the frame pointer, ordinary frame-slot accesses, and pointer-value accesses.

Close-readiness assessment:

- The original missing callee-saved GPR `slot_placement` authority bucket is
  gone for the representative rows.
- Remaining failures are outside idea 626's narrow source-authority route. The
  current RV64 dynamic object guard intentionally admits only the simple saved
  GPR placement-authority shape and still rejects dynamic frame-pointer
  fixed-slot lowering, dynamic local memory accesses, and ordinary function
  frame slots.
- Executor recommendation: idea 626 is close-ready as addressed by the current
  plan. Split any follow-up into a separate dynamic local-memory/fixed-slot
  RV64 object-lowering initiative rather than expanding this idea.

## Suggested Next

Ask the plan owner to decide whether to close idea 626 or replace the exhausted
runbook with a separate follow-up. The executor recommendation is to close idea
626 and split dynamic local-memory/fixed-slot RV64 object lowering into a new
initiative if that work is desired.

## Watchouts

- Do not treat the remaining `unsupported_stack_frame` diagnostics on the
  representative rows as missing saved-GPR placement evidence; the prepared
  dumps now show complete saved GPR placement authority.
- The next owner bucket is dynamic local-memory/fixed-slot object lowering,
  not callee-saved save-slot placement.
- No implementation files, tests, expectations, unsupported markers, allowlists,
  `plan.md`, or source idea files were touched in this packet.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
passed, 347/347 backend tests. Proof log: `test_after.log`.
