Status: Active
Source Idea Path: ideas/open/508_rv64_prepared_callee_saved_gpr_save_slot_emission.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Rebuild Save-Slot Evidence

# Current Packet

## Just Finished

Completed Step 1 of `plan.md`: rebuilt current save-slot evidence for
`tests/c/external/gcc_torture/src/20000603-1.c`.

Fresh prepared-BIR evidence is under
`build/agent_state/508_step1_save_slot_evidence/`:

- `f` has `prepared.summary @f stable_base=rsp frame_size=16
  frame_alignment=8` and a saved `gpr:s1` fact with
  `slot_placement=slot#10+stack16`, `slot_size=8`, `slot_align=8`,
  `fixed_location=yes`, and `slot_reg=gpr:s1`.
- `main` has `prepared.summary @main stable_base=rsp frame_size=8
  frame_alignment=8` and a saved `gpr:s1` fact with
  `slot_placement=slot#10+stack8`, `slot_size=8`, `slot_align=8`,
  `fixed_location=yes`, and `slot_reg=gpr:s1`.
- The same dump has nearby `fpr:fs1` save-slot evidence in `f`
  (`slot#11+stack24`, size/alignment `8`); this is a compatibility watchout,
  not owned by the GPR route.

The current object route still fails closed with:

`unsupported_stack_frame: RV64 object route requires supported prepared callee-saved GPR save slots`

The immediate rejection path is in
`src/backend/mir/riscv/codegen/object_emission.cpp`: `emit_prepared_function`
calls `make_rv64_call_frame_prologue_fragment` or
`make_rv64_stack_frame_prologue_fragment`; those call
`append_rv64_saved_callee_gpr_spills`, which delegates validation to
`rv64_saved_callee_gpr_stack_offset`. The helper currently rejects unsupported
entries by returning `std::nullopt`; in the representative, the nearby
`fpr:fs1` save in `f` reaches this GPR-only path and preserves the fail-closed
diagnostic.

## Suggested Next

Execute Step 2: trace the prepared frame-plan data entering RV64 object
emission and define the narrow helper behavior for coherent GPR saves while
keeping non-GPR save slots fail-closed or explicitly skipped only if the route
can prove that is semantically safe for this GPR packet.

## Watchouts

- Keep the route limited to RV64 object emission consuming explicit prepared
  callee-saved GPR save-slot facts.
- Do not infer save slots from testcase names, raw slot numbers, source shape,
  or register spellings.
- Do not include FPR, F128, vector, dynamic-stack, or broad fixed-frame support
  in this plan.
- `f` contains both `gpr:s1` and `fpr:fs1` saved-register facts. Step 2 must
  preserve the idea boundary: the GPR route can use the explicit `gpr:s1`
  slots, but broad FPR save/restore support remains out of scope.
- `main` exercises the call-frame prologue path with only `gpr:s1`; `f`
  exercises the non-call stack-frame prologue path with the FPR watchout.

## Proof

- `scripts/plan_review_state.py set-step --step-id 1 --step-title 'Rebuild Save-Slot Evidence'`
- Copied 424 classification artifacts into
  `build/agent_state/508_step1_save_slot_evidence/` for comparison.
- `build/c4cll --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20000603-1.c`
- `build/c4cll --codegen obj --target riscv64-linux-gnu -o build/agent_state/508_step1_save_slot_evidence/20000603-1.o tests/c/external/gcc_torture/src/20000603-1.c`
  reproduced the expected fail-closed diagnostic with `rc=2`.
- Proof log: `test_after.log`.
