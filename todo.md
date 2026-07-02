Status: Active
Source Idea Path: ideas/open/518_bir_core_model_cleanup_umbrella.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish Structure Snapshot

# Current Packet

## Just Finished

Lifecycle activation created the active runbook and execution-state skeleton
for Step 1.

## Suggested Next

Delegate Step 1: establish the BIR core model structure snapshot with
`c4c-clang-tools`, record the exact commands used, and start the durable
analysis artifact.

## Watchouts

- This idea is analysis-only; do not move declarations or definitions.
- Use AST-backed structure queries before raw long-file reading.
- Keep idea 422 producer implementation work separate from cleanup planning.
- Do not change tests, expectations, unsupported diagnostics, or pass/fail
  accounting.

## Proof

Lifecycle-only activation; no build proof required.
