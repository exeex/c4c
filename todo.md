Status: Active
Source Idea Path: ideas/open/396_rv64_object_route_terminator_fragment_lowering_refresh.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Classify Current Terminator Fragment Rejections

# Current Packet

## Just Finished

Activated `ideas/open/396_rv64_object_route_terminator_fragment_lowering_refresh.md`
after closing `ideas/closed/395_rv64_object_route_instruction_fragment_lowering.md`.

395 closed because its refreshed seed bucket no longer reproduced a current
owned `unsupported_instruction_fragment` family. 354 remains open as an
umbrella, so the next active route is the next concrete child repair bucket:
current RV64 `unsupported_terminator_fragment` failures.

## Suggested Next

Delegate Step 1 to an executor: refresh and classify the current terminator
fragment representatives, starting with `src/20020206-2.c`, and record the
first concrete terminator family plus proof command here.

## Watchouts

- Do not drop or weaken terminators to make object compilation succeed.
- Do not reconstruct missing BIR CFG semantics inside RV64 object emission.
- Route producer CFG fact gaps to their owners instead of patching them under
  396.

## Proof

Lifecycle activation only. No build or test proof was run for this
plan-owner-only reset.
