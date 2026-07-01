Status: Active
Source Idea Path: ideas/open/444_frame_slot_call_argument_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Re-establish Frame-Slot Argument Evidence

# Current Packet

## Just Finished

Completed `plan.md` Step 1 evidence for `src/20080506-2.c` from the
runtime-triage artifacts, copied into
`build/agent_state/444_step1_frame_slot_arg_evidence/`.

Named evidence:

- Semantic BIR calls `foo(ptr %lv.p1, ptr %lv.p2)` and records both arguments
  as `encoding=frame_slot` with `selection=frame_slot_address`.
- Prepared BIR `function=main`, `callsite block=0 inst=2`, records `arg0`
  from `register:s2` to `a0` and `arg1` from `frame_slot:stack+24` to `a1`.
- Prepared BIR `arg index=1` has `source_value_id=9`, `source_slot=#9`,
  `source_stack_offset=24`, `selection_source_value=%lv.p2`,
  `selection_source_slot=#8`, `selection_source_stack_offset=16`,
  `dest_reg=a1`, and `missing_frame_slot_arg_publication=yes`.
- Prepared BIR has the payload record
  `call_arg_value_publication function=main block=entry call_inst=2 arg=1`
  with `argument=%lv.p2`, `argument_value_id=9`, `object_slot=#8`,
  `object_stack_offset=16`, `payload=%lv.a`, `payload_value_id=7`,
  `destination_slot=#8`, and `destination_stack_offset=16`.
- Emitted RV64 `main` materializes arg0 with `addi a0,sp,8`, then overwrites
  the arg1 backing slot from stale `s1` (`mv t1,s1`; `sd t1,16(sp)`) before
  `addi a1,sp,16` and the call to `foo`.

## Suggested Next

Execute Step 2: trace the producer path that emits the arg1 payload record while
still setting `missing_frame_slot_arg_publication=yes`, then trace RV64 call
lowering consumption of the `call_arg_stack_to_register` move for
`source_encoding=frame_slot` / `arg.source_selection=frame_slot_address`.

## Watchouts

- Publish and consume prepared frame-slot argument authority directly; do not
  infer payloads in RV64 from testcase names, source shape, function names,
  local variable names, block indexes, or qemu signal `139`.
- Do not change expectations, allowlists, unsupported markers, runtime
  comparison, or pass/fail accounting.
- Keep generic prepared-value operand materialization in the closed idea `443`.
- Keep inline asm tied-output/result materialization outside this route.
- If the evidence proves missing upstream frame-slot payload authority, stop
  and report the producer gap instead of reconstructing it in RV64.
- Step 1 found payload authority for arg1, so Step 2 should distinguish the
  missing publication marker from the existing payload record instead of
  treating the source fact as absent.
- The emitted stale edge to explain is the `mv t1,s1`; `sd t1,16(sp)` sequence
  between arg0 materialization and `addi a1,sp,16`.

## Proof

Evidence-only proof; no build or runtime proof required by the packet.

- Hook state aligned with
  `scripts/plan_review_state.py set-step --step-id 1 --step-title 'Re-establish Frame-Slot Argument Evidence'`.
- Copied the focused 423 artifacts into
  `build/agent_state/444_step1_frame_slot_arg_evidence/`.
- `test_after.log` contains the `rg`/`sed` evidence checks for semantic BIR,
  prepared BIR, the emitted RV64 call setup, and archived return-code evidence.
- `git diff --check -- todo.md` passed after this update.
