Status: Active
Source Idea Path: ideas/open/562_bir_direct_call_semantic_boundary_evidence.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce Direct-Call Boundary Evidence

# Current Packet

## Just Finished

No executor packet has run for this active plan yet.

## Suggested Next

Start Step 1 from `plan.md`: reproduce one retained direct-call representative,
preferably `src/20000717-1.c` or `src/pr67226.c`, and record the command,
diagnostic, failing call, and dump paths here.

## Watchouts

- Keep this route evidence-first until the missing direct-call fact is named.
- Do not make RV64 call lowering changes before BIR or prepared ownership is
  proven.
- Do not use expectation rewrites, unsupported downgrades, diagnostic renames,
  or named-case shortcuts as progress.

## Proof

Lifecycle activation only. No build or code validation has run for this plan.
