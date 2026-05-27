# AArch64 ALU Prepared Authority Repair Runbook

Status: Active
Source Idea: ideas/open/51_aarch64_alu_prepared_authority_repair.md
Supersedes active route: ideas/open/49_aarch64_dispatch_value_materialization_prepared_authority_repair.md is parked open after Step 8 exposed an ALU-owned stale-home blocker.

## Purpose

Turn `src/backend/mir/aarch64/codegen/alu.cpp` into a consumer of prepared
value-home, storage, memory-access, scalar-publication, edge source-producer,
move-bundle, branch/return, and select-chain facts instead of rediscovering
scalar source and result authority locally.

## Goal

Repair ordinary AArch64 scalar ALU stale-home reads by replacing local
same-block load-local producer and publication recovery with shared prepared
authority.

## Core Rule

Prefer shared prepared scalar publication/source-producer authority over local
same-block producer walks, load/store alias scans, raw move-bundle scans, or
expectation downgrades.

## Read First

- `ideas/open/51_aarch64_alu_prepared_authority_repair.md`
- `src/backend/mir/aarch64/codegen/alu.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- Prepared authority definitions and lookup helpers for:
  - `PreparedValueHome`
  - `PreparedStoragePlanValue`
  - `PreparedValueHomeLookups`
  - `PreparedAddressingFunction`
  - `PreparedScalarPublicationPlan`
  - `PreparedEdgePublicationSourceProducerLookups`
  - `PreparedEdgePublicationSourceProducer`
  - `PreparedMoveBundle`
  - `PreparedMovePhase::BeforeReturn`

## Current Targets

- `make_prepared_scalar_alu_record`
- `make_prepared_scalar_unary_record`
- `make_prepared_scalar_operand`
- `make_prepared_scalar_result_operand`
- `make_prepared_scalar_load_source`
- `find_same_block_load_local_producer_index`
- `has_intervening_store_to_local_load_source`
- `make_unpublished_load_local_source_operand`
- `find_return_abi_register`
- `find_return_chain_register`
- `lower_scalar_select_publication`

## Non-Goals

- Do not change arithmetic opcode spelling, immediate optimizations,
  accumulator/direct-register paths, mul/div/rem scratch ordering, 32-bit
  sign/zero extension, or register-read hazard checks except to consume shared
  prepared facts.
- Do not implement calls, comparison, memory, or dispatch-value-materialization
  repairs under this plan.
- Do not push call-argument producer logic into ALU without an explicit shared
  query contract.
- Do not treat target-local scratch ordering as duplicate semantic authority
  by itself.
- Do not claim helper renames, line-count reduction, expectation rewrites, or
  unsupported-test downgrades as capability progress.

## Working Model

- ALU operand/result records should remain consumers of prepared value-home and
  storage facts.
- Scalar load source recovery should consume a prepared memory-access lookup by
  result value id/name instead of scanning `PreparedAddressingFunction::accesses`
  by spelling.
- Same-block unpublished `load_local` producer recovery should be owned by a
  shared scalar-publication or source-producer query keyed by source value and
  consumer instruction index.
- Before-return retargeting should consume prepared move or return-chain
  authority instead of raw move-bundle or forward BIR scans.
- Direct-global select-chain scalar publication should consume the shared
  select-chain materialization authority used by other non-edge consumers.

## Execution Rules

- Start from the remaining dispatch close-readiness failures:
  `c_testsuite_aarch64_backend_src_00164_c` and
  `c_testsuite_aarch64_backend_src_00204_c`.
- Establish a focused baseline for the old four-test family before further
  backend surgery:
  `00164`, `00176`, `00181`, and `00204`.
- Split the stale-home seam into small probes under `tests/backend/case/`
  before changing shared ALU authority.
- Keep probes semantic and same-feature, not named-case shortcuts for one
  c-testsuite file.
- Each code-changing step needs fresh build or compile proof. The supervisor
  chooses the exact proving subset.
- If a probe proves the remaining failure is in `calls.cpp` rather than
  ordinary ALU operand/source publication, stop and route through
  `ideas/open/52_aarch64_calls_prepared_authority_repair.md` instead of
  widening this plan.

## Steps

### Step 1: Establish the remaining stale-home baseline

Goal: make the current failure family explicit before touching ALU lowering.

Primary target: `todo.md` proof state and the supervisor-selected c-testsuite
subset

Actions:

- Run or delegate the focused four-test baseline:
  `c_testsuite_aarch64_backend_src_00164_c`,
  `c_testsuite_aarch64_backend_src_00176_c`,
  `c_testsuite_aarch64_backend_src_00181_c`, and
  `c_testsuite_aarch64_backend_src_00204_c`.
- Confirm the expected current shape: `00176` and `00181` pass after the
  select-chain label-identity repair; `00164` and `00204` still fail at
  runtime from stale or uninitialized ALU operands.
- Record the exact observed stale homes, producer values, and consumer
  instructions in `todo.md`.

Completion check:

- `todo.md` records the fresh baseline command, result, and the first concrete
  ALU stale-home sequence to reduce.

### Step 2: Add focused backend probes for unpublished load-local ALU operands

Goal: split the remaining failure seam into repo-local probes before changing
ALU authority.

Primary target: `tests/backend/case/`

Actions:

- Add one minimal probe where an unpublished same-block `load_local` feeds an
  ordinary scalar ALU operand after another materialization event.
- Add a nearby same-feature probe that varies the consumer shape, such as a
  second binary operator, chained ALU consumer, or call boundary before the ALU
  use.
- Keep expected output behavior-focused; do not encode c-testsuite-specific
  names, instruction labels, or temporary register choices.
- Use the probes to classify whether the stale read is selected by ALU operand
  publication, call argument materialization, or a shared source-producer gap.

Completion check:

- At least two focused probes exist under `tests/backend/case/`, and `todo.md`
  records which route owns the stale-home decision.

### Step 3: Route scalar load source lookup through prepared memory authority

Goal: stop `make_prepared_scalar_load_source` from using local value-spelling
scans as load source authority.

Primary target: `make_prepared_scalar_load_source`

Actions:

- Consume an existing prepared memory-access lookup by result value id/name if
  one already exists.
- Add a narrow shared lookup only if `PreparedAddressingFunction::accesses`
  cannot answer the needed query without a local scan in ALU.
- Preserve stack/global scalar load behavior while moving the lookup authority
  out of `alu.cpp`.

Completion check:

- Scalar load source recovery no longer linearly scans
  `PreparedAddressingFunction::accesses` by value spelling in ALU, with proof
  for the focused probes and the relevant c-testsuite subset.

### Step 4: Replace same-block load-local producer recovery

Goal: remove ALU-owned same-block load-local producer and no-intervening-store
logic as the source of unpublished operand truth.

Primary target: `find_same_block_load_local_producer_index`,
`has_intervening_store_to_local_load_source`, and
`make_unpublished_load_local_source_operand`

Actions:

- Consume or add a prepared scalar-publication/source-producer query keyed by
  source value and consumer instruction index.
- Use the shared query to decide whether an unpublished load-local producer can
  be read from prepared memory/source authority instead of from its assigned
  result home.
- Remove or fail-close local same-block producer/no-intervening-store scans
  after the shared path exists.

Completion check:

- Ordinary scalar ALU operands for unpublished same-block load-local producers
  consume shared authority, and the focused probes plus `00164`/`00204` no
  longer show stale-home operand reads.

### Step 5: Repair before-return and return-chain authority

Goal: keep ALU return retargeting anchored in shared prepared move or
return-chain facts.

Primary target: `find_return_abi_register` and `find_return_chain_register`

Actions:

- Add or consume a prepared before-return move lookup by source value id and
  destination register bank.
- Add a prepared return/result-chain publication query only if the move lookup
  does not cover the remaining return-chain behavior.
- Remove raw move-bundle and forward name-chain scans once shared authority is
  available.

Completion check:

- Before-return ABI retargeting and any surviving return-chain behavior consume
  shared prepared authority, with proof for affected return routes.

### Step 6: Route ALU select-chain publication through shared select authority

Goal: align `lower_scalar_select_publication` with the shared non-edge
select-chain materialization authority.

Primary target: `lower_scalar_select_publication`

Actions:

- Consume the shared scalar select-chain materialization query selected for
  non-edge consumers.
- Remove direct-global select-chain shortcuts whose only proof is a narrow
  named case.
- Preserve behavior for direct-global select-chain scalar publication through
  shared authority.

Completion check:

- Direct-global select-chain scalar publication consumes shared select-chain
  materialization authority, with proof for a direct-global route and a nearby
  same-feature route.

### Step 7: Classify remaining stale-home ownership

Goal: decide whether the remaining `00164`/`00204` stale-home family is still
owned by ALU prepared-authority repair, by calls/call-boundary publication, or
by a missing shared value-home/source-producer query.

Primary target: ownership trace in `todo.md`, using the remaining `00164`
post-`%t106` values `%t109`, `%t110`, `%t111`, and `%t112` as the first
reduction target.

Actions:

- Do not close this plan or add another ALU-local fallback while the focused
  stale-home family remains at the 4/6 split.
- Trace the prepared homes, scalar publications, call-clobber behavior, and ALU
  operand materialization for `%t109`, `%t110`, `%t111`, and `%t112`.
- Determine where the stale operand decision is made:
  - ALU-owned operand/source materialization still selecting a stale home.
  - Call-boundary or call-argument publication invalidating or failing to
    republish the prepared home.
  - Shared prepared value-home/source-producer authority missing a query needed
    by multiple consumers.
- If the trace proves the owner is call-boundary or call-argument
  publication, stop and route through
  `ideas/open/52_aarch64_calls_prepared_authority_repair.md` instead of
  widening this plan.
- If the trace proves a missing shared query is the owner, define the smallest
  shared-authority packet and the proof subset before implementation.
- Regenerate the supervisor-selected canonical `test_after.log` before any
  closure or acceptance decision; the current workspace may not have the root
  proof log that `todo.md` previously referenced.

Completion check:

- `todo.md` records the ownership classification, the traced prepared-home and
  publication path for `%t109`/`%t110`/`%t111`/`%t112`, the conditional route
  decision, and the next bounded packet. Closure remains blocked unless the
  stale-home family is green or formally reclassified outside ALU scope.
