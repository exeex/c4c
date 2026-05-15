Status: Active
Source Idea Path: ideas/open/246_prepared_aggregate_va_arg_access_plan.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Define Prepared Aggregate Access Plan Carrier

# Current Packet

## Just Finished

Lifecycle switch activated prerequisite idea 246 after idea 243 Step 4 stopped
on missing prepared/shared aggregate access-plan authority.

## Suggested Next

Execute Step 1 by defining the prepared aggregate `va_arg` access-plan carrier
and completeness checks without adding selected AArch64 aggregate
machine-node consumption.

## Watchouts

- Do not reconstruct aggregate source selection, size/alignment, copy extent,
  or `va_list` progression in AArch64 target lowering.
- Keep selected aggregate `va_arg` machine-node consumption parked in idea 243
  until this prerequisite closes.
- Preserve fail-closed diagnostics for missing or incomplete aggregate
  access-plan facts.

## Proof

Lifecycle-only switch. No code validation was run.
