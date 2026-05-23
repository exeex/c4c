Status: Active
Source Idea Path: ideas/open/aarch64-codegen-cpp-family-consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Audit CPP Family Merge Candidates

# Current Packet

## Just Finished

Activated `ideas/open/aarch64-codegen-cpp-family-consolidation.md` as the
active lifecycle plan. No implementation work has started.

## Suggested Next

Execute `plan.md` Step 1: audit current `.cpp` families, classify merge
candidates and non-targets, and choose the first implementation merge target.

## Watchouts

- Keep the source idea stable unless durable intent changes.
- Do not merge large semantic lowering modules just to reduce file count.
- Do not hide target-independent logic in AArch64 during consolidation.
- Keep slices behavior-preserving and reviewable.

## Proof

Lifecycle-only activation; no build or test proof required.
