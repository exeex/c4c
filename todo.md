Status: Active
Source Idea Path: ideas/open/185_phase_e_route2_select_chain_view_consumer_migration.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Handoff For Review Or Next Lifecycle Decision

# Current Packet

## Just Finished

Step 5 of `plan.md` prepared the lifecycle handoff for the completed Route 2
ALU select-chain consumer migration.

The selected consumer was the AArch64 scalar ALU control-publication
`select.result` path lowered through `lower_scalar_control_value_instruction(...)`
and `lower_scalar_select_publication(...)`. That consumer now asks the local
`route2_control_select_chain_materialization(...)` adapter first. The adapter
uses `mir::find_bir_select_chain_identity(...)` to recover Route 2 semantic
facts for select-root identity, root instruction index, scalar materialization
eligibility, and direct-global dependency presence.

The route-first behavior is intentionally narrow: when Route 2 reports an
available scalar select-chain materialization with direct-global dependency,
the ALU publication path emits the existing select-chain materialization
sequence. When Route 2 is absent, ineligible, or cannot map the root value back
to prepared value names, the path falls back to
`prepare::find_prepared_scalar_select_chain_materialization(...)`. Direct-global
absent scalar selects keep the ordinary `csel` path.

Prepared fallback and oracle surfaces remain public and active:
- `prepare::find_prepared_scalar_select_chain_materialization(...)`
- `prepare::find_prepared_direct_global_select_chain_dependency(...)`
- `prepare::find_prepared_store_source_direct_global_select_chain_dependency(...)`
- the existing prepared lookup/helper coverage used as the route/prepared
  agreement oracle

The final focused proof for the completed migration passed, and the accepted
Step 4 proof was rolled forward to `test_before.log`.

Lifecycle decision: idea 185 is closure-ready for the source idea's scoped goal
of migrating exactly one selected AArch64 consumer to Route 2 facts while
retaining prepared fallback/oracle behavior. Remaining prepared APIs and
consumers are residual surfaces for separate future Route 2 migration work, not
unfinished work inside this runbook.

## Suggested Next

Supervisor should run reviewer or plan-owner lifecycle handling for idea 185
closure. No implementation packet is suggested by this handoff.

## Watchouts

Do not interpret this slice as broader Route 2 prepared API deletion,
route-wide consumer migration, or aggregate contraction. Memory/store, call,
publication-planning, return, and other AArch64 select-chain/direct-global
consumers still rely on prepared surfaces unless separately migrated.

`test_baseline.new.log` remains a rejected full-suite baseline candidate from
earlier work and is not accepted proof for this lifecycle decision.

## Proof

No Step 5 build or test was run; this packet only updates the canonical
handoff summary in `todo.md`.

Final focused proof already passed and was rolled forward to `test_before.log`:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_aarch64_prepared_scalar_alu_records|backend_aarch64_scalar_alu_records|backend_aarch64_scalar_record_contract)$'; } > test_after.log 2>&1`

Accepted proof log: `test_before.log`.
