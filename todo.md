Status: Active
Source Idea Path: ideas/open/aarch64-codegen-header-family-consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Consolidate Calls Headers

# Current Packet

## Just Finished

Activation created the active runbook for `ideas/open/aarch64-codegen-header-family-consolidation.md`.

## Suggested Next

Execute Step 1 from `plan.md`: consolidate `calls_*.hpp` declarations into
`calls.hpp`, update calls-family include users, delete empty calls shard
headers, and prove with a fresh build plus supervisor-selected backend tests.

## Watchouts

- Do not merge `.cpp` files in this plan.
- Do not move responsibilities between BIR, prealloc, MIR, and AArch64 codegen.
- Keep declarations grouped by old shard subtopic inside the family header.
- Treat any remaining `calls_*.hpp` survivor as an explicit exception that must
  be recorded here.
- Do not start the CPP-family consolidation idea.

## Proof

Lifecycle-only activation. No build or test proof required for this packet.
