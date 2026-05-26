Status: Active
Source Idea Path: ideas/open/11_aarch64_calls_file_consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Audit Remaining Calls File Roles

# Current Packet

## Just Finished

Lifecycle activation created the active runbook for Step 1. No implementation
packet has run yet.

## Suggested Next

Delegate Step 1 from `plan.md`: audit remaining AArch64 calls files, classify
safe consolidation candidates, identify files that must not be merged, and
record the focused proof set.

## Watchouts

- This idea is behavior-preserving consolidation only.
- Do not move new semantic authority, alter dispatch behavior, rewrite ABI
  classification, or weaken tests.
- Leave unrelated transient `review/` files untouched.

## Proof

Lifecycle-only activation; no build or test proof required.
