Status: Active
Source Idea Path: ideas/open/247_explicit_got_materialization_policy.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate The Policy Owner

# Current Packet

## Just Finished

Lifecycle switch completed: idea 233 is parked on the missing explicit GOT
policy blocker recorded by commit `236be6f41`, and prerequisite idea 247 is now
active to add a real compiler-owned GOT-required policy source.

## Suggested Next

Execute Step 1 from `plan.md`: inspect TargetProfile, CLI option transfer, BIR
global metadata, and prepared address-materialization inputs, then choose the
narrowest explicit owner for GOT-required policy before any AArch64 GOT
selection work resumes.

## Watchouts

- Do not infer GOT from symbol spelling, `is_extern` alone, fixture names, or
  downstream assembler relocation enums.
- Do not reopen direct page+low12 global/string-constant or label selection
  behavior except for shared carrier compatibility.
- Do not implement terminal GOT printing until structured policy reaches
  prepared and selected records.

## Proof

Lifecycle-only switch. No implementation proof was required, and canonical
regression logs were not modified.
