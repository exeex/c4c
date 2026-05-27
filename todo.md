Status: Active
Source Idea Path: ideas/open/51_aarch64_alu_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit ALU Recovery Sites

# Current Packet

## Just Finished

Step 1: Audit ALU Recovery Sites completed as a read-only audit.

Findings grouped into bounded repair packets:

- Scalar ALU operand/result records: `make_prepared_scalar_alu_record`,
  `make_prepared_scalar_unary_record`, `make_prepared_scalar_operand`, and
  `make_prepared_scalar_result_operand` are prepared consumers already. They
  read `PreparedValueHome` through `find_prepared_scalar_value_home` and
  `PreparedStoragePlanValue` through `find_prepared_scalar_storage`, then keep
  AArch64-specific register/immediate/spill spelling local. No replacement
  authority is missing for this family.
- Scalar load source: `make_prepared_scalar_load_source` no longer scans
  `PreparedAddressingFunction::accesses` in `alu.cpp`; it calls
  `prepare::find_prepared_memory_access_by_result_value_name`. That helper is
  still a shared inline linear scan over `accesses` keyed only by
  `result_value_name`, with uniqueness-by-null behavior. Replacement authority
  needed: an indexed prepared memory-access lookup keyed by result value name
  or value id, preferably carried in `PreparedFunctionLookups`, so ALU consumes
  a shared lookup instead of a scan-shaped helper.
- Unpublished same-block load-local source: the old ALU-local
  `find_same_block_load_local_producer_index` and
  `has_intervening_store_to_local_load_source` names are absent. ALU now calls
  `prepare::find_prepared_same_block_load_local_source_producer` through
  `find_prepared_load_local_source_producer`, and
  `make_unpublished_load_local_source_operand` only converts the returned
  prepared source access into an AArch64 memory operand. Remaining concern is
  inside the shared helper: it still falls back to result-value memory-access
  scans and owns the intervening-store/range-overlap safety check. Replacement
  authority is present for ALU consumption; a future shared-query cleanup may
  be useful, but the ALU-local duplicate recovery is not the next blocker.
- Return ABI and return chain: `find_return_abi_register` uses
  `prepare::find_prepared_before_return_abi_move_by_source_and_destination_bank`
  with move-bundle lookups and only falls back inside shared lookup code when
  indexes are unavailable. `find_return_chain_register` uses
  `prepare::find_prepared_return_chain_terminal_value` plus indexed value-home
  lookup before reusing the before-return ABI move query. No ALU raw
  move-bundle scan or forward BIR producer walk remains in this file.
- Direct-global select-chain publication: `lower_scalar_select_publication`
  calls `prepare::find_prepared_direct_global_select_chain_dependency` and
  passes the result into shared `emit_select_chain_value_to_register`. This
  removes the local direct-global named-case decision from ALU, but the
  fallback still depends on shared source-producer/select-chain traversal
  rather than a dedicated non-edge scalar select-chain materialization plan.
  Missing shared query: a prepared scalar select-chain materialization lookup
  for non-edge/non-store consumers, probably reusing the existing
  `PreparedDirectGlobalSelectChainDependency` shape until a fuller plan exists.

## Suggested Next

Implement the Step 2 scalar load source packet: add an indexed prepared
memory-access lookup keyed by result value name or value id, route
`make_prepared_scalar_load_source` through it, and retire the current
result-value scan helper from the ALU path.

## Watchouts

Do not treat the existing inline
`find_prepared_memory_access_by_result_value_name` scan as the final authority
repair. Also avoid expanding the next packet into unpublished load-local,
return-chain, or select-chain behavior; those are separate bounded families
after the memory-access lookup is made index-backed.

## Proof

Read-only audit only. No build or test was required or run; no
`test_after.log` was produced for this audit-only packet.
