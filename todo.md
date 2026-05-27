# Current Packet

Status: Active
Source Idea Path: ideas/open/47_aarch64_dispatch_edge_copies_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 6
Current Step Title: Consolidate proof and regression guard expectations

## Just Finished

Step 6 close-scope proof consolidation completed for idea 47. The fresh
close-scope proof in `test_after.log` covers the repaired AArch64/backend route
surface and passed 39/39 tests.

Coverage recorded for the idea-47 authority repairs:

- Source-producer authority: Step 2 routed materialization and hazard producer
  lookup through prepared source-producer facts instead of local
  predecessor/name-shaped recovery.
- Null-publication fallback behavior: the remaining null-publication
  prepared-memory-access route is preserved as a non-direct path, while direct
  prepared publications fail closed when required authority is absent.
- Load-local source-memory materialization: Step 3 routes direct load-local
  source materialization through prepared source-memory facts and value-home
  authority.
- Block-entry move-bundle matching: Step 4 requires prepared move-bundle and
  edge-publication authority for block-entry redundancy and parallel-copy source
  matching.
- Select-chain/direct-global shared authority: Step 5 routes late
  call-argument materialization through prepared call-plan/source-value
  authority plus the shared direct-global select-chain dependency query.

## Suggested Next

Supervisor can compare canonical `test_before.log`/`test_after.log` with
`c4c-regression-guard` if desired, then hand the active plan to the plan owner
for close/archive handling of
`ideas/open/47_aarch64_dispatch_edge_copies_prepared_authority_repair.md`.

## Watchouts

`find_edge_named_producer` and `unique_branch_predecessor_context` remain
defined/exported, but current `src/` and `tests/` references show no call sites
outside the helper body/declarations. Treat any reintroduced caller as a close
blocker unless it is routed through a shared prepared query. Load-local
materialization still allows the non-direct/null-publication
prepared-memory-access route; the direct prepared-publication route now
requires available source-memory authority. Block-entry edge-copy suppression
no longer drops same-register or source-memory moves without prepared
edge-publication authority. Indirect callee materialization still has a
separate direct-global select-chain check outside the call-argument path
covered by Step 5; this Step 6 proof records the call-argument route as covered
by the shared direct-global dependency query without expanding the packet scope.

## Proof

Ran:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_store_source_publication_plan|backend_aarch64|backend_codegen_route_aarch64|backend_cli_aarch64)' | tee test_after.log`

Result: passed, 39/39 close-scope tests green. `test_after.log` now contains
the delegated Step 6 proof. No implementation files, tests, `plan.md`, source
idea files, or `test_before.log` were modified by this packet.
