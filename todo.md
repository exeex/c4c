Status: Active
Source Idea Path: ideas/open/509_rv64_fixed_prepared_stack_frame_emission.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Rebuild Fixed-Frame Evidence

# Current Packet

## Just Finished

Lifecycle activation created the active runbook and execution-state skeleton
for Step 1 of `plan.md`.

## Suggested Next

Execute Step 1: rebuild the fixed-frame evidence for `src/20030209-1.c`,
confirm the prepared frame plan, and identify the current RV64 object-route
rejection point.

## Watchouts

- Keep the route limited to RV64 object emission consuming explicit prepared
  fixed-frame facts.
- Do not infer frame size, stack offsets, widths, alignments, or slot layout
  from testcase names, raw slot counts, source shape, or magic constants.
- Do not include dynamic stack, FPR, F128, vector, varargs, producer-layout, or
  broad ABI support in this plan.

## Proof

- Pending executor proof for Step 1.
