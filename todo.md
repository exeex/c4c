Status: Active
Source Idea Path: ideas/open/509_rv64_fixed_prepared_stack_frame_emission.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Rebuild Fixed-Frame Evidence

# Current Packet

## Just Finished

Step 1 of `plan.md` rebuilt the fixed-frame evidence for
`tests/c/external/gcc_torture/src/20030209-1.c` without code or test edits.
The idea 424 handoff artifacts and fresh Step 1 probes agree: prepared BIR
publishes a coherent fixed frame for `main` with `frame_size=80000`,
`frame_alignment=8`, `has_dynamic_stack=no`, and `fixed_slots_use_fp=no`.
The prepared stack layout contains 10000 fixed slots for `main`; sample slot
facts are `slot #0 offset=0 size=4 align=4`, `slot #1 offset=8 size=8 align=8`,
and `slot #9999 offset=79992 size=8 align=8`. The prepared frame plan
republishes the same 10000-slot order from `slot_id=#0` through
`slot_id=#9999`.

The current RV64 object route still rejects the representative before object
emission with:
`unsupported_stack_frame: RV64 object route requires a supported prepared stack frame`.
The code rejection point is
`src/backend/mir/riscv/codegen/object_emission.cpp:10280`: `prepared_function_to_object_function`
calls `rv64_object_stack_frame_size(addressing, frame_plan, prepared.stack_layout)`,
and the helper returns `std::nullopt` because it rounds the prepared frame to
16-byte alignment and only accepts sizes that fit a signed 12-bit immediate.
For this representative, the 80000-byte fixed frame is outside that current
consumer limit. The route has not yet reached the later FPR save-slot rejection;
prepared FPR save slots for `fs1` and `fs2` remain a watchout, not part of this
fixed-frame packet.

Artifacts:
- `build/agent_state/424_step2_infrastructure_classification/20030209-1/`
- `build/agent_state/509_step1_fixed_frame_evidence/dump-prepared-bir.txt`
- `build/agent_state/509_step1_fixed_frame_evidence/prepared-frame-evidence.rg.txt`
- `build/agent_state/509_step1_fixed_frame_evidence/prepared-stack-layout-sample.rg.txt`
- `build/agent_state/509_step1_fixed_frame_evidence/prepared-stack-layout-slot-count.txt`
- `build/agent_state/509_step1_fixed_frame_evidence/prepared-frame-slot-order-count.txt`
- `build/agent_state/509_step1_fixed_frame_evidence/codegen-obj.txt.stderr`
- `build/agent_state/509_step1_fixed_frame_evidence/codegen-obj.txt.rc`

## Suggested Next

Execute Step 2: trace the minimal RV64 object-emission consumer path for
prepared fixed-frame facts. Start at `rv64_object_stack_frame_size`, then follow
stack adjustment and frame-slot offset consumers that currently require signed
12-bit immediates so the next code packet can validate coherent fixed frames
and materialize large stack adjustments from prepared facts.

## Watchouts

- Keep the route limited to RV64 object emission consuming explicit prepared
  fixed-frame facts.
- Do not infer frame size, stack offsets, widths, alignments, or slot layout
  from testcase names, raw slot counts, source shape, or magic constants.
- Do not include dynamic stack, FPR, F128, vector, varargs, producer-layout, or
  broad ABI support in this plan.
- The representative prepared plan also lists saved registers
  `gpr:s1`, `fpr:fs1`, and `fpr:fs2`. Step 1 confirms the current generic
  rejection occurs before `diagnose_unsupported_prepared_saved_register_bank`;
  do not use FPR save-slot support as the first fixed-frame repair.

## Proof

- `scripts/plan_review_state.py set-step --step-id 1 --step-title 'Rebuild Fixed-Frame Evidence'`
- `./build/c4cll --dump-prepared-bir --target riscv64-linux-gnu --mir-focus-function main tests/c/external/gcc_torture/src/20030209-1.c > build/agent_state/509_step1_fixed_frame_evidence/dump-prepared-bir.txt 2> build/agent_state/509_step1_fixed_frame_evidence/dump-prepared-bir.txt.stderr`
  - return code: `0`
- `./build/c4cll --codegen obj --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20030209-1.c -o build/agent_state/509_step1_fixed_frame_evidence/20030209-1.o > build/agent_state/509_step1_fixed_frame_evidence/codegen-obj.txt 2> build/agent_state/509_step1_fixed_frame_evidence/codegen-obj.txt.stderr`
  - return code: `2`, expected current rejection for Step 1 evidence
- `rg` filters over the prepared dump recorded the exact frame facts and counts
  under `build/agent_state/509_step1_fixed_frame_evidence/`.
- `test_after.log` is not applicable for this evidence-only packet; the
  supervisor requested artifact-local probe logs instead.
