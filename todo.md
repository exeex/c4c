Status: Active
Source Idea Path: ideas/open/51_aarch64_alu_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit ALU Recovery Sites

# Current Packet

## Just Finished

Activation created the runbook and initialized execution state for Step 1:
Audit ALU Recovery Sites.

## Suggested Next

Begin Step 1 by auditing `src/backend/mir/aarch64/codegen/alu.cpp` recovery
sites and recording the replacement prepared authority or missing shared query
for each bounded repair family.

## Watchouts

Do not edit implementation code as part of activation. Routine audit findings
and proof notes should stay in this file unless the runbook contract itself
needs lifecycle repair.

## Proof

Lifecycle-only activation. No build or test proof was run.
