Status: Active
Source Idea Path: ideas/open/65_aarch64_target_owner_relocation.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Target-Owner Relocation Candidates

# Current Packet

## Just Finished

Plan owner activated idea 65 into `plan.md` and initialized this execution
state for Step 1.

## Suggested Next

Step 1 - Inventory Target-Owner Relocation Candidates: inspect the
dispatch-family AArch64 helper surfaces, classify precise target-owner
destinations, identify public-surface shrink opportunities, and record the
first coherent relocation packet without implementation edits.

## Watchouts

This is a target-owner relocation plan, not a shared-authority migration.
Preserve target-local clobber handling, register spelling, instruction
spelling, frame-address rendering, scratch hazards, and retry routing. Do not
bundle idea 67's local-slot null-offset probe into this plan.

## Proof

No proof run for lifecycle-only activation.
