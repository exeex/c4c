# AArch64 ALU Prepared Authority Repair Runbook

Status: Active
Source Idea: ideas/open/51_aarch64_alu_prepared_authority_repair.md

## Purpose

Repair duplicate scalar ALU authority in AArch64 lowering so `alu.cpp`
consumes shared prepared value-home, storage, memory, scalar-publication,
edge source-producer, move-bundle, select-chain, and return facts instead of
reconstructing semantic identity locally.

## Goal

Make `src/backend/mir/aarch64/codegen/alu.cpp` rely on prepared authority for
scalar ALU operands/results, scalar load sources, unpublished load-local
sources, before-return ABI retargeting, return-chain behavior, and
direct-global select-chain publication, adding narrow shared lookups only where
the existing prepared facts cannot answer the ALU consumer query.

## Core Rule

ALU lowering may choose AArch64 arithmetic spelling, immediates, scratch
registers, accumulator paths, and emission order, but semantic authority must
come from prepared facts rather than raw value-name scans, same-block
load/store recovery, raw move-bundle scans, forward BIR name-chain walks, or
direct-global select-chain named-case logic.

## Read First

- `ideas/open/51_aarch64_alu_prepared_authority_repair.md`
- `src/backend/mir/aarch64/codegen/alu.cpp`
- Prepared value-home, storage-plan, addressing, scalar-publication,
  source-producer, move-bundle, select-chain, and return-chain lookup helpers.
- Recent closed memory and store-source authority ideas when shared prepared
  query shape is unclear, especially idea 50 and the store-source publication
  plan work it references.

## Current Targets

- `src/backend/mir/aarch64/codegen/alu.cpp`
- Shared prepared lookup surfaces only when an existing prepared contract cannot
  represent the ALU consumer's semantic query.

## Non-Goals

- Do not change arithmetic opcode spelling, immediate optimizations,
  accumulator or direct-register paths, mul/div/rem scratch ordering, 32-bit
  sign/zero extension, or register-read hazard checks except to consume shared
  facts.
- Do not claim target-local scratch ordering, emitted-scalar reuse, or typed
  register conversion as duplicate semantic authority by itself.
- Do not fold comparison fused-compare, call materialization, dispatch, or
  memory lowering repair into this plan.
- Do not weaken supported-path expectations or downgrade failing tests to
  unsupported without explicit user approval.

## Working Model

- `alu.cpp` is the AArch64 scalar ALU lowering owner and should remain the place
  that converts prepared scalar facts into AArch64 arithmetic, select, and
  return-adjacent instructions.
- Prepared value-home and storage records should provide operand/result meaning
  before lowering reaches `alu.cpp`.
- Prepared addressing, scalar-publication, source-producer, move, select-chain,
  and return facts should answer semantic recovery questions that are currently
  reconstructed inside ALU lowering.
- If the current prepared lookup is missing a key needed by ALU lowering, add
  the smallest shared query that exposes the already-prepared fact.
- Delete or narrow obsolete local recovery once the prepared fact is consumed;
  do not keep the old authority under a renamed helper.

## Execution Rules

- Keep each code-changing step behavior-preserving apart from authority-source
  repair.
- Use `todo.md` to record audit findings and proof results; do not rewrite the
  source idea for routine discoveries.
- Prefer one bounded repair family per executor packet.
- Reject testcase-shaped matching, named-case shortcuts, expectation rewrites,
  or helper-only renames as progress.
- For each implementation step, run at least a fresh build or compile proof and
  the supervisor-selected narrow ALU/backend tests.
- Escalate to broader backend validation before closure because ALU lowering
  affects scalar arithmetic, select publication, load-source recovery, and
  return-adjacent paths.

## Step 1: Audit ALU Recovery Sites

Goal: map each duplicate authority path in `alu.cpp` to an existing or missing
prepared fact before making implementation edits.

Primary target:

- `src/backend/mir/aarch64/codegen/alu.cpp`

Actions:

- Inspect `make_prepared_scalar_alu_record`,
  `make_prepared_scalar_unary_record`, `make_prepared_scalar_operand`, and
  `make_prepared_scalar_result_operand` to confirm they remain consumers of
  prepared value-home and storage facts.
- Inspect `make_prepared_scalar_load_source` for scans over
  `PreparedAddressingFunction::accesses` by `result_value_name`.
- Inspect `find_same_block_load_local_producer_index`,
  `has_intervening_store_to_local_load_source`, and
  `make_unpublished_load_local_source_operand` for same-block load-local
  producer and safety recovery.
- Inspect `find_return_abi_register` and `find_return_chain_register` for raw
  move-bundle scans and forward ALU producer walks.
- Inspect `lower_scalar_select_publication`,
  `select_chain_contains_direct_global_load`, and
  `emit_select_chain_value_to_register` for direct-global select-chain
  fallback authority.
- Record in `todo.md` the exact replacement authority or missing lookup needed
  for each family.

Completion check:

- The duplicate recovery sites are grouped into bounded repair packets with
  named prepared facts or explicit missing shared queries.

## Step 2: Repair Scalar Load Source Authority

Goal: route scalar load source recovery through prepared memory or source
lookup authority instead of value-spelling scans over prepared accesses.

Primary target:

- `src/backend/mir/aarch64/codegen/alu.cpp`

Actions:

- Replace `PreparedAddressingFunction::accesses` scans keyed by
  `result_value_name` with an existing prepared memory-access lookup if one can
  answer result value id/name.
- If no lookup exists, add a narrow shared prepared memory-access query keyed
  by result value and route `make_prepared_scalar_load_source` through it.
- Keep stack/global scalar home materialization and AArch64 operand emission
  behavior unchanged.
- Remove obsolete local scanning once the prepared lookup is consumed.

Completion check:

- Scalar load source recovery no longer linearly scans prepared accesses by
  value spelling, with focused proof covering affected scalar load ALU paths.

## Step 3: Repair Unpublished Load-Local Producer Authority

Goal: replace same-block load-local producer and no-intervening-store recovery
with shared scalar-publication or source-producer facts.

Primary target:

- `src/backend/mir/aarch64/codegen/alu.cpp`

Actions:

- Route unpublished load-local source discovery through
  `PreparedScalarPublicationPlan`, edge source-producer lookups, or another
  shared source-producer query keyed by value plus consumer instruction index.
- Add the smallest shared query only when existing prepared facts have the
  information but no ALU-suitable consumer lookup.
- Preserve the safety property currently enforced by the no-intervening-store
  check without keeping AArch64-local alias scans as semantic authority.
- Remove or narrow obsolete same-block producer helpers after call sites
  consume the shared fact.

Completion check:

- Same-block load-local producer and safety logic is owned by shared prepared
  authority rather than local ALU scans.

## Step 4: Repair Return ABI And Return-Chain Authority

Goal: make before-return retargeting and surviving return-chain behavior
consume prepared move or return-chain authority.

Primary target:

- `src/backend/mir/aarch64/codegen/alu.cpp`

Actions:

- Replace raw `move_bundles` scans in `find_return_abi_register` with an
  existing prepared before-return move lookup if one can answer source value id
  plus destination register bank.
- If no lookup exists, add a narrow shared before-return move query and route
  `alu.cpp` through it.
- Audit `find_return_chain_register`; delete it if prepared return move lookup
  covers the behavior, or replace it with a prepared return/result-chain query
  if the behavior is still required.
- Preserve ABI register spelling and final return-adjacent ALU emission
  behavior.

Completion check:

- Before-return ABI retargeting and any surviving return-chain behavior are
  keyed by prepared authority rather than raw move-bundle scans or forward BIR
  name-chain walks.

## Step 5: Repair Select-Chain Scalar Publication Authority

Goal: make direct-global select-chain scalar publication consume the same
shared select-chain materialization authority used by other non-edge consumers.

Primary target:

- `src/backend/mir/aarch64/codegen/alu.cpp`

Actions:

- Route `lower_scalar_select_publication` direct-global behavior through an
  existing prepared select-chain materialization query if one exists.
- If no query exists, add a narrow shared prepared scalar select-chain
  materialization query for non-edge and non-store consumers.
- Remove or narrow direct-global select-chain named-case checks once the shared
  query owns the semantic decision.
- Preserve AArch64 select-chain instruction spelling and publication register
  behavior.

Completion check:

- Direct-global select-chain scalar publication is represented by shared
  prepared select-chain materialization authority rather than ALU-local
  direct-global fallback logic.

## Step 6: Final Validation And Closure Check

Goal: prove the source idea's acceptance criteria are satisfied without
testcase overfit or expectation downgrades.

Actions:

- Run the supervisor-selected narrow ALU/backend proof for each repaired
  family.
- Run broader backend validation sufficient to catch scalar ALU, load-source,
  select-publication, and return-adjacent regressions.
- Review the diff for new prepared-access value-spelling scans, same-block
  load/store alias scans, raw move-bundle scans, forward BIR name-chain walks,
  direct-global select-chain named-case expansion, unsupported-test rewrites,
  or helper-only renames.

Completion check:

- All source acceptance criteria are satisfied, regression proof is current,
  and any remaining work is outside this source idea's scope.
