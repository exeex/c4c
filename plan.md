# AArch64 Named Handoff Materializer Cleanup Runbook

Status: Active
Source Idea: ideas/open/709_aarch64_named_handoff_materializer_cleanup.md

## Purpose

Remove executable route-record authority from AArch64 MIR materialization while
preserving target-local instruction and ABI realization.

## Goal

Make AArch64 materializers consume the common named and prepared handoff views,
fail closed when required authority is unavailable, and retire semantic route
and route-index dependencies from the target codegen owner.

## Core Rule

Migrate consumers to existing common authority. Do not recreate route analysis,
route indexes, or target-local semantic fallbacks under new names.

## Read First

- `ideas/open/709_aarch64_named_handoff_materializer_cleanup.md`
- `ideas/closed/727_common_prepared_return_chain_authority.md`
- `ideas/closed/728_prepared_return_chain_shape_authority_decomposition.md`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- the common named/prepared query interfaces established by ideas 706 and 727

## Current Scope

- AArch64 dispatch, calls, globals, ALU, comparison, select, publication, and
  value materialization.
- Existing AArch64 instruction-dispatch, call-boundary, branch-control,
  current-block/join, scalar-ALU, and memory-operand proof surfaces.
- Removal of semantic route records and target-local route indexes from the
  AArch64 materialization owner.

## Non-Goals

- No common producer or query-contract redesign.
- No x86 or RV64 consumer migration.
- No target instruction or ABI policy changes.
- No expectation weakening, testcase-shaped fallback, or assembly-only proof.
- Debug vocabulary may remain only when it cannot affect lowering.

## Working Model

- Common named/prepared views own semantic handoff authority.
- AArch64 owns only legal instruction selection and ABI realization.
- Missing or inconsistent prepared authority must fail closed.
- Route-labelled state must not select executable behavior.

## Execution Rules

- Resume at Step 2.1 and consume the successor-linked production authority
  proven by ideas 727 and 728 without rebuilding any relation in AArch64.
- Require `Available` only for an actual successor-linked relation; preserve
  fail-closed `StructurallyIncomplete` for terminal-only inputs with no links.
- Replace consumers with existing typed/common queries; do not copy producer
  reasoning into target helpers.
- Keep each packet behavior-preserving and prove more than one narrow fixture
  before retiring a fallback.
- Stop for lifecycle review if an existing common query cannot express required
  authority without a producer-contract change.

## Ordered Steps

### Step 1: Inventory and migrate AArch64 dispatch authority

Goal: establish the bounded first consumer migration without duplicating route
semantics.

Primary target: `src/backend/mir/aarch64/codegen/dispatch.cpp`

Completion check:

- The migrated dispatch family has no executable route dependency, retains
  target-local behavior, and passes focused AArch64 proof.

### Step 2.1: Delete the ALU return-chain reconstruction

Goal: consume common traversal-attached return-chain authority and remove the
target-local reconstruction and lookup fallback.

Primary target: `src/backend/mir/aarch64/codegen/alu.cpp`

Actions:

- Consume only the instruction traversal event's
  `PreparedObjectReturnChainClassification` and require `Available` with a
  complete relation.
- Use its terminal return-ABI home/register placement and first non-chain
  operand home for target-local register realization.
- Delete `find_prepared_return_chain_facts`, its move-bundle/scalar-producer
  walk, and its fallback `make_prepared_function_lookups` construction.
- Do not walk move bundles, successor homes, scalar producers, or binary
  operands in AArch64 to rediscover or complete the relation.
- Fail closed when attached authority is absent or inconsistent.
- Prove the corrected ALU/return behavior with the supervisor-selected build
  and focused scalar-ALU plus return-path tests.

Completion check:

- The AArch64 consumer uses only traversal-attached common authority,
  `find_prepared_return_chain_facts` and its lookup fallback are gone, and
  focused ALU/return proof is green.

### Step 2.2: Migrate direct dispatch-producer consumers

Goal: remove direct same-block producer reconstruction from the remaining
dispatch-producer family.

Primary target: `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`

Actions:

- Replace direct producer discovery with the applicable traversal-attached
  common named/prepared query.
- Gate any relation not represented by an existing common query; do not derive
  it from target-local instruction or operand walks.
- Preserve target-local instruction selection and fail closed on unavailable
  or inconsistent authority.
- Prove instruction-dispatch behavior and nearby same-feature cases.

Completion check:

- The dispatch-producer family contains no executable producer reconstruction
  or lookup fallback, and focused dispatch proof is green.

### Step 2.3: Migrate select and comparison consumers

Goal: remove local value-home lookup rebuilding from select/comparison
materialization as one bounded control/value family.

Primary target: `src/backend/mir/aarch64/codegen/select_materialization.cpp`

Actions:

- Replace local value-home lookup construction with existing attached common
  authority.
- Gate missing common-query authority instead of rebuilding homes or semantic
  relations locally.
- Preserve comparison/select instruction policy and fail closed when required
  authority is unavailable.
- Prove branch-control and current-block/join behavior across nearby select and
  comparison cases.

Completion check:

- Select/comparison materialization consumes attached common authority only,
  and its focused control-flow proof is green without expectation weakening.

### Step 2.4: Migrate floating-point and general value consumers

Goal: remove named-producer discovery from the remaining value-materialization
family.

Primary target: `src/backend/mir/aarch64/codegen/fp_value_materialization.cpp`

Actions:

- Replace named-producer discovery with the existing attached common query.
- Apply the same authority gate to directly implicated general value or
  publication consumers, one coherent file family per executor packet.
- Do not infer missing producer relations from target-local operands or
  instruction shape.
- Prove scalar/value and publication behavior for each affected family.

Completion check:

- The migrated value family has no executable named-producer discovery or
  reconstructed lookup, and nearby same-feature proof remains green.

### Step 2.5: Migrate memory consumers

Goal: retire the remaining target-local lookup rebuilding in the memory family.

Primary target: `src/backend/mir/aarch64/codegen/memory.cpp`

Actions:

- Consume traversal-attached common lookups and remove local lookup builders as
  their final consumers disappear.
- Gate any missing common relation and return to lifecycle review rather than
  extending idea 709 into producer/query redesign.
- Preserve memory instruction policy and prove nearby memory-operand behavior.

Completion check:

- Memory materialization uses attached common authority, missing authority
  fails closed, and focused memory-operand proof is green.

### Step 2.6: Migrate remaining call consumers

Goal: retire lookup rebuilding from the remaining call and ABI materializers.

Primary target: remaining call materializers identified by the retirement
search.

Actions:

- Consume traversal-attached common lookups and remove local lookup builders as
  their final call consumers disappear.
- Gate any missing common relation instead of deriving it from ABI operands or
  instruction shape.
- Preserve AArch64 ABI realization and prove call-boundary behavior.

Completion check:

- Remaining call materializers use attached common authority, missing authority
  fails closed, and focused call-boundary proof is green.

### Step 2.7: Migrate global and publication consumers

Goal: retire lookup rebuilding from the remaining global/publication family.

Primary targets: remaining global and publication materializers identified by
the retirement search.

Actions:

- Migrate the directly related global/publication consumers as one coherent
  data-publication family.
- Consume traversal-attached common lookups and gate any missing common
  relation rather than rebuilding it locally.
- Preserve target-local relocation and publication policy and prove nearby
  global/publication behavior.

Completion check:

- Global/publication consumers use attached common authority, missing authority
  fails closed, and their focused proof is green.

### Step 3: Prove retirement and disposition

Goal: demonstrate that semantic AArch64 materialization no longer depends on
route vocabulary or recreated indexes.

Actions:

- Search the scoped semantic owner for remaining executable route/index use.
- Classify any surviving route-labelled text as non-semantic debug vocabulary
  owned by idea 712, or remove it when local and safe.
- Run the supervisor-selected broader AArch64 validation checkpoint.
- Review the complete slice against the source idea reject signals.

Completion check:

- The retirement guard is zero for semantic AArch64 materialization, broad
  behavior proof is green, and no route/index recreation or expectation
  weakening remains.
