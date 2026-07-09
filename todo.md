Status: Active
Source Idea Path: ideas/open/648_rv64_call_arg_frame_slot_address_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh The Call-Argument Source Evidence

# Current Packet

## Just Finished

Step 1 diagnostic refresh completed for
`tests/c/external/gcc_torture/src/20000722-1.c`. Evidence is under
`build/agent_state/648_step1_call_arg_frame_slot_evidence/`, with summary at
`build/agent_state/648_step1_call_arg_frame_slot_evidence/summary.md`.

Prepared BIR confirms the representative call is `bar:entry` instruction 10 to
`foo`, argument 0. The exact prepared call-plan fact is
`arg.source_selection=local_frame_address_materialization` with
`selection_source_value=%lv._clit_`, `selection_source_value_id=4`,
`selection_source_slot=#0`, `selection_source_stack_offset=0`,
`selection_source_size=8`, `selection_source_align=8`,
`selection_materialization_block=entry`, and
`selection_materialization_inst=10`. The selected ABI destination is
`dest_placement=gpr:call_argument#0/w1 dest_reg=x0`, mapping to RV64 `a0`.

The stale register-home copy path is confirmed at the prepared move-bundle
level: `move_bundle phase=before_call authority=none block_index=0
instruction_index=10` contains `reason=call_arg_register_to_register`, source
home `reg=x21` / RV64 `s2`, and destination `reg=x0` / RV64 `a0`.

Fresh RV64 object-route diagnostics currently fail before object/disassembly
emission with `[RV64_C4C_OBJ_COMPILE_FAIL]` and
`unsupported_local_memory_access: RV64 object route requires prepared
frame-slot or pointer-value base-plus-offset local memory addressing`. Because
no fresh object was emitted, current disassembly does not confirm an emitted
`mv a0,s2`; older pre-existing artifacts still show the historical `mv a0,s2`
shape but are not treated as fresh proof for this packet.

The current owner remains RV64 consumption/lowering of prepared
`arg.source_selection=local_frame_address_materialization`, not string-label
pointer admission, stack layout, ABI convention, branch lowering, or runtime
support.

## Suggested Next

Execute Step 2: locate the RV64 call-argument lowering boundary that consumes
prepared call argument source selection. Inspect the text asm path in
`src/backend/mir/riscv/codegen/prepared_call_emit.cpp` around
`emit_riscv_frame_slot_address_argument` and the argument lowering branch, and
inspect the object path in
`src/backend/mir/riscv/codegen/object_emission.cpp` around
`append_rv64_prepared_local_frame_address_call_argument_source` plus the
same-module call argument emission path. Record whether the current
`unsupported_local_memory_access` object-route blocker is the first boundary to
repair before the stale register-copy emission can be reproven.

## Watchouts

- Do not reopen string-constant local-memory admission.
- Do not special-case `src/20000722-1.c`, `%lv._clit_`, `foo`, `s2`, or `a0`.
- Do not infer frame-slot address materialization from source spelling, stack
  offsets, final assembly, testcase identity, or diagnostics.
- Treat `reason=call_arg_register_to_register` as the stale prepared
  move-bundle path, but do not claim a fresh emitted `mv a0,s2` unless the
  object route reaches disassembly again.
- The fresh object route currently stops at `unsupported_local_memory_access`;
  Step 2 should decide whether that local-memory blocker is inside the same
  RV64 call-argument consumption boundary or must be split before implementing
  frame-slot address argument materialization.
- Do not edit expectations, unsupported markers, allowlists, timeouts,
  runtime-comparison policy, or pass/fail accounting.

## Proof

No build/ctest proof was required. Ran focused prepared-BIR, semantic BIR,
RV64 asm, RV64 trace/MIR, and RV64 object-harness diagnostics into
`build/agent_state/648_step1_call_arg_frame_slot_evidence/`. Also ran
`cmake --build --preset default`, which reported no work to do, to confirm the
diagnostic binary was current. Did not create or overwrite `test_after.log`.
