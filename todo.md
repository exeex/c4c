Status: Active
Source Idea Path: ideas/open/395_rv64_object_route_instruction_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Classify Instruction Fragment Families

# Current Packet

## Just Finished

Lifecycle activation created the runbook for
`ideas/open/395_rv64_object_route_instruction_fragment_lowering.md`.

## Suggested Next

Delegate Step 1 to an executor: classify the current
`unsupported_instruction_fragment` family using `src/20000223-1.c`, the idea
406 residual `src/20020225-2.c`, and nearby same-diagnostic cases from current
RV64 gcc_torture backend artifacts.

## Watchouts

- Do not absorb stack-frame or parameter-home work owned by idea 398.
- Do not infer missing producer facts inside RV64 object emission.
- Do not claim progress from filename-specific lowering, expectation rewrites,
  allowlist filtering, or diagnostic-only churn.

## Proof

Not run. This is lifecycle-only activation; implementation proof belongs to
the delegated executor packet.
