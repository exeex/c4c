Status: Active
Source Idea Path: ideas/open/444_frame_slot_call_argument_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Re-establish Frame-Slot Argument Evidence

# Current Packet

## Just Finished

Activation initialized this execution state for `plan.md` Step 1.

## Suggested Next

Execute Step 1: re-establish the frame-slot call-argument evidence for
`src/20080506-2.c` from the runtime triage handoff, then identify the prepared
publication records and emitted RV64 call setup edges that must stop passing
stale argument state.

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

## Proof

Lifecycle activation only. No build or test proof required yet.
