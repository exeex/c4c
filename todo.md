Status: Active
Source Idea Path: ideas/open/398_rv64_object_route_stack_frame_and_param_home_edges.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Classify Current Stack Frame And Home Rejections

# Current Packet

## Just Finished

Lifecycle activation created the active runbook from
`ideas/open/398_rv64_object_route_stack_frame_and_param_home_edges.md`.

## Suggested Next

Execute Step 1: refresh the current RV64 gcc_torture stack-frame and
parameter-home bucket for `930603-1.c`, `20001017-1.c`, `va-arg-21.c`, and
supervisor-selected nearby same-bucket representatives. Classify the first
concrete frame, callee-saved save-slot, parameter-home, FPR-home, variadic
admission, or producer missing-fact family before implementation.

## Watchouts

- Do not fabricate stack-frame, callee-saved, parameter-home, or variadic facts
  in RV64 object emission.
- Do not reimplement the ABI classifier in the object emitter.
- Route producer-side missing facts to lifecycle review instead of absorbing
  them into target lowering.
- Do not use filename-specific branches, expectation rewrites, or allowlist
  filtering as progress.

## Proof

No execution proof yet. This is lifecycle activation only.
