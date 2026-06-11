Status: Active
Source Idea Path: ideas/open/196_prepared_function_lookups_ownership_readiness_audit.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Produce the durable ownership/readiness map

# Current Packet

## Just Finished

Completed Step 4 of `plan.md`: created the durable
`PreparedFunctionLookups` ownership/readiness map at
`docs/bir_prealloc_fusion/prepared_function_lookups_ownership_readiness_map.md`.
The artifact carries forward the Step 1 reader inventory, Step 2 ownership
classification, and Step 3 adapter/contraction prerequisites.

The map includes all seven lookup groups:

- `call_plans`
- `address_materializations`
- `memory_accesses`
- `move_bundles`
- `value_homes`
- `edge_publications`
- `edge_publication_source_producers`

It separates semantic route facts from target/prepared policy, compatibility
adapters, and retained oracle/debug surfaces. It also explicitly records that
no lookup group is ready for deletion, privatization, or aggregate-wide
replacement from this audit alone.

## Suggested Next

Execute Step 5 final validation for idea 196. Confirm the durable artifact
satisfies the source idea acceptance criteria, every lookup group has a
classification and reader/blocker notes, no implementation/tests/expectations
changed, and the audit stayed out of broad `PreparedBirModule` retirement or
draft 155 work.

## Watchouts

- The durable artifact intentionally rejects deletion, privatization, broad
  BIR-owned clone/rename-only progress, and aggregate-wide replacement from
  this audit alone.
- Future implementation work must stay one consumer or adapter boundary at a
  time and preserve prepared fallback plus target-local policy.
- Route-native candidates are semantic subfact candidates only; ABI,
  addressing, home/storage, move scheduling, wrapper construction, and final
  emission records remain target/prepared-owned unless a separate
  implementation idea proves otherwise.
- Prepared printer/debug, `backend_prepared_lookup_helper`, target-wrapper,
  and AArch64 lookup-threading coverage remain retained oracle surfaces.

## Proof

Docs/lifecycle-only artifact packet. No build/test proof was required and no
`test_after.log` was generated. Validation/searches performed:

- Read `AGENTS.md`, active `plan.md`, source idea 196, and current `todo.md`.
- Read `src/backend/prealloc/prepared_lookups.hpp` to confirm the seven
  `PreparedFunctionLookups` lookup groups.
- Ran targeted `rg` scans for `PreparedFunctionLookups`, `prepared_lookups`,
  all seven direct lookup group names, and cross-target wrapper references in
  `src`, `tests`, and `docs/bir_prealloc_fusion`.
- Cross-checked the artifact against
  `docs/bir_prealloc_fusion/prepared_diagnostics_oracle_replacement_plan.md`
  and `docs/bir_prealloc_fusion/cross_target_route_view_reuse_map.md`.
- Wrote
  `docs/bir_prealloc_fusion/prepared_function_lookups_ownership_readiness_map.md`
  with columns for lookup group, current readers, ownership classification,
  route-view replacement status, fallback/oracle surfaces, adapter need,
  blockers, and future readiness.
