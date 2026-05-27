# AArch64 ALU Prepared Authority Repair

## Goal

Repair duplicate scalar ALU authority in
`src/backend/mir/aarch64/codegen/alu.cpp` by consuming prepared value-home,
storage, memory-access, scalar-publication, edge source-producer, move-bundle,
and branch/return facts, and by adding missing shared queries for scalar
loads, return ABI, return chains, and select-chain materialization.

## Why This Exists

The audit found that `alu.cpp` mostly owns target-local arithmetic emission,
but still includes scans for memory access by result value, same-block
load-local producers, before-return ABI registers, forward return chains, and
direct-global select-chain scalar publication.

## Owned File

- `src/backend/mir/aarch64/codegen/alu.cpp`

## Duplicated Helpers And Fallback Paths

- `make_prepared_scalar_alu_record`, `make_prepared_scalar_unary_record`,
  `make_prepared_scalar_operand`, and `make_prepared_scalar_result_operand`
  should remain consumers of prepared value-home and storage facts.
- `make_prepared_scalar_load_source` scans
  `PreparedAddressingFunction::accesses` by `result_value_name`.
- `find_same_block_load_local_producer_index`,
  `has_intervening_store_to_local_load_source`, and
  `make_unpublished_load_local_source_operand` recover same-block load-local
  producer and safety facts for unpublished scalar loads.
- `find_return_abi_register` scans raw move bundles for before-return ABI
  destinations.
- `find_return_chain_register` scans forward through ALU producers to prove a
  value feeds the block return.
- `lower_scalar_select_publication` uses
  `select_chain_contains_direct_global_load` and
  `emit_select_chain_value_to_register` for direct-global select-chain
  fallback.

## Shared Facts To Consume Or Add

- Consume `PreparedValueHome`, `PreparedStoragePlanValue`,
  `find_prepared_value_home`, `PreparedValueHomeLookups`, and storage-plan
  value facts for ALU operands/results.
- Add or consume a prepared memory-access lookup by result value id/name if
  ALU must materialize stack/global scalar homes outside the original
  instruction index.
- Add or consume a prepared scalar load-local source-producer query by value
  plus consumer instruction index, either in `PreparedScalarPublicationPlan` or
  a shared source-producer lookup.
- Add or consume a before-return move lookup by source value id and
  destination register bank for `PreparedMoveBundle`/
  `PreparedMovePhase::BeforeReturn`.
- Add a prepared return/result-chain publication query only if return move
  lookup repair does not cover `find_return_chain_register`.
- Add or consume a prepared scalar select-chain materialization query for
  non-edge/non-store consumers.

## Out Of Scope

- Do not change arithmetic opcode spelling, immediate optimizations,
  accumulator/direct-register paths, mul/div/rem scratch ordering, 32-bit
  sign/zero extension, or register-read hazard checks except to consume shared
  facts.
- Do not claim target-local scratch ordering is duplicate authority by itself.
- Do not combine this idea with comparison fused-compare or calls
  materialization repair.

## Acceptance Criteria

- ALU structured records continue to consume prepared value-home/storage facts
  without raw value-name or register-allocation scans.
- Scalar load source recovery no longer linearly scans
  `addressing->accesses` by value spelling once a shared lookup exists.
- Same-block load-local producer/no-intervening-store logic is replaced by a
  shared scalar-publication or source-producer query.
- Before-return ABI retargeting and any surviving return-chain behavior are
  owned by shared prepared move or return-chain authority.
- Direct-global select-chain scalar publication consumes the same shared
  select-chain materialization authority chosen for other non-edge consumers.

## Closure Note

Closed after Step 6 final validation. The active runbook repaired the ALU
authority families covered by this idea and the final audit found no new
expectation downgrades, unsupported rewrites, prepared-access value-spelling
scans in ALU, same-block load/store alias scans in ALU, raw move-bundle scans
or forward BIR return-chain walks in ALU, or direct-global select-chain
named-case expansion in ALU.

Close-gate validation used matching before/after runs of the final validation
scope: backend subset plus targeted c-testsuite cases 00164, 00196, and 00207.
Both runs kept the same known failure shape: backend failures
`backend_aarch64_instruction_dispatch` and
`backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor`,
targeted `00164` passing, and known targeted `00196` runtime mismatch plus
`00207` timeout.

## Reviewer Reject Signals

- Reject new scans over `PreparedAddressingFunction::accesses` keyed by value
  spelling.
- Reject deeper same-block load/store alias scans in ALU as source recovery.
- Reject raw `move_bundles` scans for return ABI publication or result
  retargeting after a shared lookup is available.
- Reject forward name-chain scans through BIR binaries as durable return-chain
  authority.
- Reject direct-global/select-chain named-case expansion, expectation
  downgrades, or helper renames claimed as capability progress.
