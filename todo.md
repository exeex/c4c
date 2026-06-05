Status: Active
Source Idea Path: ideas/open/110_call_planning_frame_address_materialization_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Prove Current Consumer And Producer Reachability

# Current Packet

## Just Finished

Activation created this execution scratchpad for Step 1 of `plan.md`.

## Suggested Next

Execute Step 1: use AST-backed caller/callee checks and read-only producer
inspection to map each `find_local_frame_address_source()` fallback branch to
the explicit prepared frame-slot materialization route it should use.

## Watchouts

- Do not edit implementation during Step 1 unless the supervisor delegates a
  code-changing packet.
- Do not remove or weaken stack-layout legacy bootstrap producers.
- Treat testcase-shaped shortcuts and expectation downgrades as route drift.

## Proof

Activation validation only. No build or backend tests are required until a
code-changing executor packet begins.

