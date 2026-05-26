Status: Active
Source Idea Path: ideas/open/34_aarch64_store_source_publication_planning.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit AArch64 Store-Source Decisions

# Current Packet

## Just Finished

Lifecycle activation created the active runbook for Step 1.

## Suggested Next

Execute Step 1 by auditing `src/backend/mir/aarch64/codegen/memory_store_sources.*`,
choosing the first target-neutral store-source decision to migrate, and recording
the selected shared authority plus narrow proof command here.

## Watchouts

- Do not merge `memory_store_sources.*` into `memory.cpp` during this semantic
  migration.
- Do not add AArch64-only rediscovery under a new helper name.
- Do not claim progress through expectation rewrites, unsupported downgrades,
  or fixture-specific matching.

## Proof

No proof run for lifecycle-only activation.
