Status: Active
Source Idea Path: ideas/open/517_residual_scalar_f32_f64_cast_object_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Rebuild Residual Cast Row Evidence

# Current Packet

## Just Finished

Lifecycle activation created this execution scratchpad for Step 1 of
`plan.md`.

## Suggested Next

Delegate Step 1 - Rebuild Residual Cast Row Evidence to an executor.

## Watchouts

Split before implementation if the three fresh rows do not all have explicit
prepared scalar-cast facts. Do not infer missing bank, type, home, opcode, or
materialization authority in RV64 object emission, and keep F128/helper ABI,
local-memory, aggregate/byval, stack-frame, branch/select, call/return, and
`conversion.c` work out of this route.

## Proof

Lifecycle-only activation. No build or test proof was run.
