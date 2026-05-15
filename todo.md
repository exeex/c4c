Status: Active
Source Idea Path: ideas/open/248_prepared_i128_runtime_helper_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect I128 Helper Authority Gap

# Current Packet

## Just Finished

Lifecycle split completed: idea 236 is parked on a prepared/shared i128
runtime-helper authority blocker, and idea 248 is now active as the
prerequisite runbook.

## Suggested Next

Execute Step 1 from `plan.md`: inspect i128 helper-required operation shapes,
generic call facts, lane carriers, memory-return needs, and helper
clobber/resource policy, then record the first implementation packet target
and focused proof subset before code changes.

## Watchouts

- Do not proceed to idea 236 Step 7 printer work while helper boundary
  authority is missing.
- Do not synthesize helper calls directly in AArch64 dispatch from
  `BinaryInst` or `CastInst` opcodes.
- Do not hard-code helper callees, fixed `x0`/`x1` lane ownership, register
  adjacency, or clobber policy inside AArch64 target lowering.
- Generic retained-call `PreparedCallPlan` facts are not enough unless they are
  tied to i128 source operation identity and low/high lane authority.

## Proof

Lifecycle-only split/reset. No build or test proof was required, and proof
logs were not modified.
