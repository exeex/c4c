# 186 Phase E Route 3 memory semantic view consumer migration

## Goal

Switch one semantic memory source reader to the Route 3 memory/access identity
view.

## Why This Exists

Phase D separated semantic memory/source identity from target address policy.
This idea migrates one consumer that needs semantic identity while keeping
addressing and operand formation prepared/AArch64-owned.

Source: `docs/bir_prealloc_fusion/phase_d_mir_consumer_switch_plan.md`.

## In Scope

- One AArch64 memory, globals, or load-local source recovery path that needs
  semantic access identity.
- Route 3 access identity, same-block global-load identity, or load-local
  stored-value source relationships.
- Prepared memory/access, global-load, same-block global-load, and load-local
  source helpers as public oracles.

## Out Of Scope

- Address base kind, offsets, stack/frame objects, TLS/global relocation data,
  pointer materialization, base-plus-offset legality, volatile/address-space
  behavior, or final memory operand records.
- Deleting prepared memory/access APIs.
- Copying `PreparedAddress` or `PreparedMemoryAccess` wholesale into BIR.

## Acceptance Criteria

- The selected consumer reads Route 3 semantic memory facts where available.
- Tests prove agreement for global-load, same-block global-load, local-stored
  source, absent-route, and non-semantic target-address-only cases.
- Address-materialization tests continue to prove target policy remains
  prepared-owned.

## Completion Note

Closed after migrating the selected AArch64 indirect-callee stored-value source
consumer to the narrow Route 3 stored-source view with prepared fallback
preserved. Post-commit review found the route aligned with this idea, with no
testcase overfit or plan rewrite needed. Canonical four-test proof passed 4/4,
supervisor broader backend validation passed 180/180, and the close-time
regression guard passed in non-decreasing mode for the lifecycle-only closure.

## Reviewer Reject Signals

- The slice moves target addressing or operand-formation policy into BIR.
- Missing Route 3 data is recovered by rescanning BIR or matching names.
- Prepared memory helpers are hidden before residual consumers are migrated.
- Tests are weakened or reduced to the selected happy path.
- Helper reshaping is claimed as semantic migration.
