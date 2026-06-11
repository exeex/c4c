Status: Active
Source Idea Path: ideas/open/185_phase_e_route2_select_chain_view_consumer_migration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select One Route 2 Consumer

# Current Packet

## Just Finished

Step 1 of `plan.md` selected one bounded Route 2 consumer for the next
implementation packet:
`src/backend/mir/aarch64/codegen/alu.cpp` in the control-publication select
path around `prepare::find_prepared_scalar_select_chain_materialization(...)`
for `select.result` before `emit_select_chain_value_to_register(...)`.

Selected prepared fallback/oracle:
`prepare::find_prepared_scalar_select_chain_materialization(...)` with the
current block label, `context.bir_block`, `select.result`, and
`instruction_index + 1U`. Its answer currently supplies
`root_value_name`, `root_instruction_index`, scalar materialization
availability, and `PreparedDirectGlobalSelectChainDependency`. Keep this
helper public as fallback/oracle; do not delete or hide the prepared
select-chain/direct-global helpers.

Selected Route 2 query/view surface:
reuse the existing `mir::BirSelectChainIdentityRequest` view over
`Route2SelectChainValueQuery` / `route2_find_select_chain_value_record(...)`,
matching the pattern already used by
`dispatch_value_materialization.cpp::route2_scalar_select_chain_materialization`.
The implementation should ask for `select.result` at
`instruction_index + 1U`, require `root_is_select`,
`root_instruction_index`, and `scalar_materialization_available`, and map only
the Route 2 direct-global presence/root facts into the prepared-shaped data
needed by the existing emitter. If Route 2 has no valid answer, the packet can
fall back to the prepared helper or fail closed exactly as the current prepared
path does.

Focused coverage to use/extend:
`backend_prepared_lookup_helper` already covers direct-global present and
absent roots, scalar-eligible/ineligible roots, nested select-chain behavior,
mismatched type/name boundaries, immediate roots, missing roots, and
Route 2 index/prepared agreement. Pair it with AArch64 ALU coverage:
`backend_aarch64_prepared_scalar_alu_records`,
`backend_aarch64_scalar_alu_records`, and
`backend_aarch64_scalar_record_contract`; add or tighten a selected-control
select case there if the existing tests do not observe the direct-global
select-chain path.

Rejected candidates for this packet: the AArch64 call indirect-callee
direct-global path in `calls.cpp` because it risks call ABI/publication policy,
and memory/store/publication paths because they risk memory operand formation,
storage/home selection, or multiple-consumer migration.

## Suggested Next

Implement Step 2 for only the selected AArch64 ALU control-publication select
consumer: add or reuse a narrow Route 2 adapter beside the existing prepared
fallback, then switch that one prepared scalar select-chain read to route-first
behavior without moving result-register selection, stack publication, emitted
record spelling, or select materialization sequence choice into Route 2.

## Watchouts

Keep the slice to this single ALU select-result read. Do not migrate the
already route-backed `dispatch_value_materialization.cpp` path, the
`dispatch_producers.cpp` direct-global predicate, AArch64 call argument or
callee paths, memory/store consumers, publication planning, printer/debug
surfaces, or tests by expectation downgrade. The Route 2 adapter must not
choose homes/registers, form memory operands, decide call/publication policy,
spell final records, rescan broad BIR, use name matching beyond the typed
request, or import return-chain behavior.

## Proof

Discovery-only packet; no build/tests run and no root-level proof log written.
Proposed implementation proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_aarch64_prepared_scalar_alu_records|backend_aarch64_scalar_alu_records|backend_aarch64_scalar_record_contract)$'`.
