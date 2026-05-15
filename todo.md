Status: Active
Source Idea Path: ideas/open/245_prepared_scalar_va_arg_access_plan.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Define Scalar Access-Plan Carrier

# Current Packet

## Just Finished

Lifecycle switch completed: idea 243 is parked on the missing prepared/shared
fact `helper_operand_homes.va_arg.scalar_access_plan`, and this prerequisite
idea is active.

## Suggested Next

Start Step 1 by defining the prepared/shared scalar `va_arg` access-plan
carrier. Keep the work in prepared/shared authority and do not implement
selected AArch64 scalar `va_arg` consumption in this prerequisite.

## Watchouts

- The access plan must cover source classification, value size/alignment,
  result-home relationship, and `va_list` progression.
- Do not reconstruct AAPCS64 access selection in AArch64 target lowering.
- Do not weaken the existing fail-closed diagnostic for
  `helper_operand_homes.va_arg.scalar_access_plan`.

## Proof

Lifecycle-only switch; no build proof run.
