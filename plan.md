# Shared Prepared Fact Query Surface Extraction Runbook

Status: Active
Source Idea: ideas/open/133_shared_prepared_fact_query_surface_extraction.md

## Purpose

Move remaining target-neutral prepared fact relationships out of local AArch64 dispatch-family rediscovery and behind a shared prepared or prealloc query owner.

## Goal

Replace at least one concrete AArch64 rediscovery site with a shared target-neutral query while preserving all AArch64 emission, register, scratch, relocation, and instruction-spelling policy locally.

## Core Rule

Extract query authority only; do not move AArch64 emission policy into shared code and do not hide predecessor rescans, BIR-name matching, or same-block named-case logic behind a generic API.

## Read First

- `ideas/open/133_shared_prepared_fact_query_surface_extraction.md`
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_lookup.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
- Existing shared owners under `src/backend/mir/prealloc/` and `src/backend/mir/prepare*`

## Current Targets

- AArch64 local rediscovery sites for prepared value-home facts
- AArch64 local rediscovery sites for edge-publication source-producer facts
- AArch64 local wrappers around current-block entry publication facts
- AArch64 local same-block materialization relationship checks
- The narrow shared owner that already owns the selected prepared or prealloc fact relationship

## Non-Goals

- Do not move AArch64 register names, scratch choices, hazard policy, relocation operands, or final instruction spelling into shared code.
- Do not replace prepared facts with predecessor rescans, BIR-name matching, or target-local reconstruction.
- Do not broadly rewrite dispatch publication or value materialization.
- Do not claim progress through helper renames, file-count cleanup, or expectation weakening.
- Do not absorb unrelated work from ideas `134` or `135` unless the selected 133 packet directly needs that same fact relationship.

## Working Model

The AArch64 dispatch family may select registers and emit instructions locally, but fact relationships such as value homes, publication producers, current-block entry publication, and same-block materialization should be queryable from shared prepared or prealloc state when they are target-neutral. Each extraction must start from an actual local rediscovery site and end with the caller asking a shared owner for the relationship directly.

## Execution Rules

- Start with source evidence: identify the local rediscovery site, the prepared/prealloc fact it reconstructs, and the existing shared owner candidate before editing.
- Prefer the smallest shared query that describes the target-neutral relationship directly.
- Keep AArch64-specific register-view selection, scratch handling, branch-fusion hooks, relocation operands, and final instruction spelling in AArch64 files.
- Do not add bridge wrappers that preserve the same local reconstruction under a new shared name.
- For each code-changing step, run `cmake --build --preset default`.
- Because shared backend query code is in scope, preserve matching canonical `test_before.log` and `test_after.log` for the same backend proof command when accepting implementation progress.
- Use `ctest --test-dir build -R '^backend_' --output-on-failure` as the default backend proof unless the supervisor delegates a narrower packet proof.

## Steps

### Step 1: Inventory Candidate Rediscovery Sites

Goal: Classify AArch64 dispatch-family sites that locally reconstruct prepared or prealloc fact relationships.

Primary Target: AArch64 dispatch-family query and materialization files.

Actions:

- Inspect the read-first AArch64 files for prepared value-home, edge-publication source-producer, current-block entry publication, and same-block materialization relationship checks.
- For each candidate, record the local helper or code block, the fact relationship it queries, current direct callers, and whether an existing shared owner already has the source data.
- Separate true target-neutral fact queries from AArch64 emission policy, register-view selection, scratch handling, relocation spelling, and hazard handling.
- Pick the first implementation packet by choosing the smallest candidate with a real shared-owner path and nearby same-feature cases to examine.

Completion Check:

- `todo.md` records an evidence table for the inspected candidates and names the selected first extraction target.
- No implementation files are edited in this step.

### Step 2: Extract One Shared Prepared Fact Query

Goal: Add or expose one shared prepared/prealloc query that replaces an actual AArch64 rediscovery site.

Primary Target: The selected shared owner under `src/backend/mir/prealloc/` or `src/backend/mir/prepare*`.

Actions:

- Add the narrow shared query API to the owner that already holds the prepared or prealloc fact data.
- Express the API in target-neutral terms; avoid AArch64 helper names and emission concepts.
- Update the selected AArch64 caller to use the shared query.
- Preserve existing AArch64 register choices, scratch behavior, relocation operands, and final instruction spelling.
- Remove or privatize only the local reconstruction that the shared query truly replaces.

Completion Check:

- At least one concrete local rediscovery site is replaced by a shared query.
- The build passes.
- The delegated backend proof passes and is recorded in `test_after.log`.

### Step 3: Examine Nearby Same-Feature Cases

Goal: Prove the extraction is not a one-case wrapper and identify whether additional candidates belong in this runbook or in narrower open ideas.

Primary Target: Nearby AArch64 call sites for the same prepared fact family.

Actions:

- Inspect nearby same-feature cases around the selected extraction target.
- Convert directly equivalent local rediscovery sites to the shared query when they use the same target-neutral relationship.
- Leave distinct select-chain or current-block-specific follow-up work to ideas `134` or `135` when it is not required for the selected 133 query.
- Record any remaining candidates that should stay as follow-up rather than expanding this packet.

Completion Check:

- Nearby same-feature cases are either converted, explicitly rejected as a different relationship, or recorded as follow-up.
- The build passes after any code changes.
- The backend proof command passes after any code changes.

### Step 4: Finalize Shared Query Contract

Goal: Confirm the new shared query surface owns fact authority without changing target-local emission behavior.

Primary Target: `todo.md` and touched shared/AArch64 interfaces.

Actions:

- Summarize the final shared query API, its owner rationale, and every AArch64 local reconstruction it replaced.
- Confirm no AArch64 emission policy, register-view selection, branch-fusion behavior, hazard policy, relocation operand, or final instruction spelling moved into shared code.
- Confirm no predecessor rescans, BIR-name matching, local fact reconstruction, unsupported downgrades, or expectation weakening were introduced.
- Preserve matching canonical regression logs for the accepted backend proof.

Completion Check:

- `todo.md` records the final query contract, converted sites, deferred follow-ups, and proof result.
- `cmake --build --preset default` passes.
- `ctest --test-dir build -R '^backend_' --output-on-failure` passes with matching before/after logs when required.
