# BIR Edge Value Flow Authority Runbook

Status: Active
Source Idea: ideas/open/16_bir_edge_value_flow_authority.md

## Purpose

Move CFG edge value-flow authority out of AArch64 dispatch and into shared BIR
semantics plus prepared edge-publication facts that future x86 and RISC-V
lowering can consume.

## Goal

Make shared BIR/prealloc preparation the source of truth for block-entry and
edge-publication value facts while keeping AArch64 responsible only for
target-specific emission and hazards.

## Core Rule

Do not replace AArch64 producer rediscovery with testcase-shaped shortcuts.
Every step must move or consume general semantic facts, not weaken expectations
or hide missing prepared records.

## Read First

- ideas/open/16_bir_edge_value_flow_authority.md
- src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp
- src/backend/mir/aarch64/codegen/dispatch_edge_copies.hpp
- src/backend/mir/aarch64/codegen/dispatch_publication.cpp
- src/backend/mir/aarch64/codegen/dispatch_publication.hpp
- src/backend/mir/aarch64/codegen/dispatch_publication_common.hpp
- src/backend/mir/aarch64/codegen/dispatch.cpp
- src/backend/mir/aarch64/codegen/traversal.cpp
- src/backend/bir/bir.hpp
- src/backend/prealloc/prepared_lookups.hpp
- src/backend/prealloc/prepared_lookups.cpp

## Current Targets

- Identify edge/block-entry value-flow facts currently inferred in AArch64.
- Add target-neutral BIR or prepared records/queries for those facts.
- Update AArch64 to consume prepared edge-publication facts instead of broad
  semantic producer rediscovery.
- Preserve AArch64-specific register names, scratch choice, addressing mode
  limits, immediate constraints, and instruction spelling in AArch64 codegen.

## Non-Goals

- Do not implement x86 or RISC-V codegen.
- Do not move target instruction spelling, register vocabulary, scratch policy,
  or encoding constraints into BIR.
- Do not rewrite call ABI lowering.
- Do not replace the whole AArch64 dispatch pipeline in one step.
- Do not claim success from line-count reduction, helper renames, or expectation
  rewrites without an authority move.

## Working Model

- BIR owns block-entry and CFG edge value-flow semantics.
- Shared prepare converts BIR value-flow semantics plus value homes into
  prepared edge-publication plans.
- A prepared edge-publication plan should identify the predecessor/successor
  relation, source value, destination home, publication phase, and enough
  ordering information for target lowering to emit moves safely.
- AArch64 consumes prepared plans and performs target-local emission, including
  register selection details, scratch handling, assembly spelling, and
  target-specific hazards.

## Execution Rules

- Prefer small behavior-preserving slices with fresh build proof.
- Add or update focused tests near the semantic authority move, not after a
  broad rewrite.
- Keep missing shared facts visible through explicit diagnostics or failing
  closed behavior; do not preserve broad AArch64 fallback rediscovery under a
  new helper name.
- When a step affects shared prepared contracts, run BIR/prealloc tests plus the
  focused AArch64 backend subset that consumes the records.
- Use `test_after.log` for executor proof unless the supervisor delegates a
  different artifact.

## Step 1: Audit Current Edge Value Flow

Goal: produce an implementation-facing map of semantic facts currently inferred
inside AArch64 edge-copy and publication lowering.

Primary targets:

- src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp
- src/backend/mir/aarch64/codegen/dispatch_publication.cpp
- src/backend/mir/aarch64/codegen/dispatch_producers.cpp
- src/backend/mir/aarch64/codegen/dispatch_lookup.cpp
- tests/backend/bir/backend_prealloc_block_entry_publications_test.cpp
- tests/backend/bir/backend_prepare_authoritative_join_ownership_test.cpp

Actions:

- Inspect predecessor producer lookup, join/select source discovery,
  block-entry publication decisions, redundant-copy filtering, and
  parallel-copy safety decisions.
- Classify each fact as shared semantic value flow, shared prepared planning, or
  target-specific emission/hazard handling.
- Record the audit in `todo.md` first; only rewrite this runbook if the audit
  proves the step boundaries are wrong.

Completion check:

- `todo.md` names the AArch64 rediscovery paths to retire, the shared facts
  needed to replace them, and the target-specific pieces that must remain in
  AArch64.
- No implementation files are changed unless the executor packet explicitly
  includes a narrow mechanical probe approved by the supervisor.

## Step 2: Define Shared Edge Publication Facts

Goal: add target-neutral BIR/prealloc representation or lookup APIs for the
edge-publication facts identified in Step 1.

Primary targets:

- src/backend/bir/bir.hpp
- src/backend/bir/bir.cpp
- src/backend/prealloc/prepared_lookups.hpp
- src/backend/prealloc/prepared_lookups.cpp
- src/backend/prealloc/prepared_printer/control_flow.cpp
- tests/backend/bir/backend_prealloc_block_entry_publications_test.cpp
- tests/backend/bir/backend_prepared_lookup_helper_test.cpp

Actions:

- Add or extend shared records/queries for predecessor/successor identity,
  source value, destination home, and publication phase.
- Keep the record target-neutral; do not encode AArch64 register vocabulary or
  instruction spelling.
- Add focused BIR/prealloc tests that prove the records express joins,
  selects, branch edges, and block-entry publications generally.

Completion check:

- Shared tests prove prepared edge-publication facts without relying on
  AArch64-only producer lookup.
- Build proof covers the changed shared backend target.

## Step 3: Consume Prepared Edge Publications in AArch64

Goal: move AArch64 edge-copy lowering to consume shared prepared facts for the
covered paths.

Primary targets:

- src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp
- src/backend/mir/aarch64/codegen/dispatch_edge_copies.hpp
- src/backend/mir/aarch64/codegen/dispatch_publication.cpp
- src/backend/mir/aarch64/codegen/dispatch_publication_common.hpp
- tests/backend/mir/backend_aarch64_prepared_branch_records_test.cpp
- tests/backend/mir/backend_aarch64_prepared_handoff_gate_test.cpp

Actions:

- Replace local semantic producer rediscovery with reads from prepared
  edge-publication records for the paths supported by Step 2.
- Leave target-local move spelling, scratch choice, register hazards, and
  AArch64 diagnostics in AArch64 codegen.
- Fail closed when a prepared fact required for an edge publication is missing.

Completion check:

- Focused AArch64 tests continue to cover join, select, branch, and block-entry
  publication behavior.
- The diff removes or narrows broad producer rediscovery instead of moving it
  behind a differently named AArch64 helper.

## Step 4: Share Reusable Copy Planning Decisions

Goal: move reusable parallel-copy and redundant-copy decisions behind shared
prepared helpers when they are independent of target instruction encoding.

Primary targets:

- src/backend/prealloc/prepared_lookups.hpp
- src/backend/prealloc/prepared_lookups.cpp
- src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp
- tests/backend/bir/backend_prepared_lookup_helper_test.cpp
- tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp

Actions:

- Identify copy-plan decisions that depend only on value homes, edge identity,
  or publication phase.
- Extract those decisions into shared helpers or prepared records.
- Keep decisions that depend on physical register hazards or instruction
  encoding in AArch64.

Completion check:

- Shared helper tests cover redundant-copy suppression and ordering facts.
- AArch64 tests prove emitted behavior is unchanged for the migrated cases.

## Step 5: Validate Authority Boundary

Goal: prove the final boundary matches the source idea and is ready for
supervisor review or closure judgment.

Primary targets:

- src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp
- src/backend/mir/aarch64/codegen/dispatch_publication.cpp
- src/backend/prealloc/prepared_lookups.cpp
- tests/backend/bir
- tests/backend/mir
- tests/backend/case

Actions:

- Inspect the final diff for remaining broad AArch64 semantic producer
  rediscovery in edge-copy lowering.
- Ensure any remaining AArch64 logic is target emission, scratch/hazard
  handling, diagnostics, or target-specific operand materialization.
- Run the supervisor-selected broader validation, likely including BIR/prealloc
  tests and focused AArch64 backend tests.

Completion check:

- AArch64 no longer owns broad semantic producer rediscovery where prepared
  edge-publication facts exist.
- Prepared representation remains target-neutral enough for future x86 and
  RISC-V consumers.
- Backend validation shows no regression in the focused AArch64 join, select,
  branch, and block-entry publication coverage.
