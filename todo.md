# Current Packet

Status: Active
Source Idea Path: ideas/open/169_route_local_identity_domain_cleanup.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Broader Backend Checkpoint

## Just Finished

Step 5 broader backend checkpoint completed for the BIR local-slot id migration
and related prepared/backend route coverage.

Checkpoint result:
- Full build passed with `cmake --build build -j`.
- Broader backend CTest regex selected 122 backend/backend-route tests; 12
  disabled MIR trace tests did not run, and all 110 runnable tests passed.
- No new failures were observed in the migrated BIR local-slot id family,
  prepared stack layout, prepared printer, semantic BIR dumps, or x86 local
  memory route tests.

Closure readiness:
- Step 1 inventory selected BIR local-slot authority as the bounded migrated
  family.
- Step 2 introduced `SlotNameId` authority for BIR local slots, local
  load/store references, local-slot memory-address bases, and sret storage,
  while retaining string spelling as display/no-id compatibility payload.
- Step 3 added direct verifier tests for duplicate ids, load/store/address
  id-name mismatches, and no-id compatibility.
- Step 4 classified remaining route-local string families and recorded
  oversized AArch64 and prealloc/out-of-SSA raw-name work as follow-up scope
  instead of expanding this runbook.

Files changed:
- `todo.md`

## Suggested Next

Supervisor lifecycle decision: call the plan owner to decide whether idea 169
is complete enough to close, should be deactivated with follow-up scope, or
needs a split before closure.

## Watchouts

- This runbook intentionally migrated one meaningful route-local family rather
  than all route-local string carriers.
- AArch64 direct LIR emitter string-keyed lowering and the larger
  prealloc/out-of-SSA raw-name helper set remain explicit follow-up scope if
  deeper typed-id migration is needed.
- Idea 170 still owns string-authority guard and allowlist machinery.
- `review/168_step4_hir_bridge_route_review.md` remains unrelated dirty state
  if present.

## Proof

Build/proof results:
- `cmake --build build -j`: pass.
- `ctest --test-dir build -j --output-on-failure -R '^(backend_|backend_codegen_route_|positive_sema_inline_backend_coord_c$)' > test_after.log 2>&1`:
  pass, 110/110 runnable tests; 12 disabled MIR trace tests did not run.
- `git diff --check -- todo.md`: pass.
