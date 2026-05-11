# Current Packet

Status: Active
Source Idea Path: ideas/open/169_route_local_identity_domain_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Fence Remaining Route-Local String Families

## Just Finished

Step 4 fenced the remaining selected route-local string families after the BIR
local-slot id migration. No production code edits were needed; existing compact
comments already distinguish display/compatibility spellings from semantic id
authority at the ambiguous BIR and prepared-backend sites.

Classifications recorded:
- Display-only:
  BIR/Prepared printers render `Value::name`, local slot spellings, block
  labels, inline-asm text, structured type spellings, and prepared register
  names for dumps or final assembly text when no lookup depends on the raw
  spelling.
- Generated local handle:
  LIR ids in `src/codegen/lir/ir.hpp`, `PreparedObjectId`, `PreparedValueId`,
  `PreparedFrameSlotId`, temporary select/phi materialization names, and x86/
  AArch64 generated labels are route-local generated handles. They are not
  source `TextId` or link-visible `LinkNameId` domains.
- Validation key:
  BIR `BlockLabelId` and `SlotNameId` are semantic validation keys when present.
  `bir_validate.cpp` rejects mismatched block-label ids, duplicate incoming ids,
  duplicate local-slot ids, and load/store/address slot-id spelling drift.
- Cross-pass local identity:
  Prepared `FunctionNameId`, `BlockLabelId`, `ValueNameId`, `SlotNameId`,
  `PreparedValueId`, and `PreparedFrameSlotId` carry local identity across
  liveness, regalloc, stack-layout, out-of-SSA, prepared control-flow, and x86
  prepared-module consumers. Prepared block-label id contracts are already used
  by guard-chain/local-slot route tests from prior slices.
- Compatibility payload:
  Raw LIR-to-BIR `ValueMap`, `GlobalTypes`, `TypeDeclMap`, local pointer/slot
  provenance maps, BIR raw block-label/slot-name spellings, string-pool names,
  `MemoryAddress::base_name`, call sret storage spelling, and AArch64 legacy
  string-keyed lowering are retained compatibility payloads where the pipeline
  still imports or matches legacy textual LIR. When a semantic id exists, these
  spellings are display or fallback payloads rather than authority.

Explicit source/link-domain check:
- No new `TextId` or `LinkNameId` use was introduced for route-local temporary
  domains in this Step 4 slice.
- Existing `LinkNameId` uses remain limited to link-visible symbol identity
  or symbol-backed addresses, not local slots, SSA values, block labels, or
  generated backend temporaries.

Follow-up scope note:
- The remaining broad legacy string-keyed areas are the AArch64 direct LIR
  emitter and the larger prealloc/out-of-SSA route helpers that still bridge
  through raw labels/value names for compatibility. Those are too large for
  this runbook packet and should be handled as separate initiatives if they
  need typed-id migration beyond the current BIR/prepared-backend fences.

Files changed:
- `todo.md`

## Suggested Next

Step 5 should run the supervisor-selected broader backend checkpoint over the
already migrated BIR local-slot id family plus the Step 3 focused tests.

## Watchouts

- This packet intentionally did not add the broad route-local string guard or
  allowlist; idea 170 owns guard machinery.
- Do not treat the raw BIR/LIR compatibility spellings as source or link
  identity. When `BlockLabelId`, `SlotNameId`, `ValueNameId`, or
  `PreparedFrameSlotId` is present, the id is the authority.
- Any future migration of AArch64 legacy string-keyed lowering or the full
  out-of-SSA raw-name helper set should be split out rather than appended to
  idea 169 Step 5.
- `review/168_step4_hir_bridge_route_review.md` remains unrelated dirty state
  if present.

## Proof

Build/proof results:
- `git diff --check`: pass.
- No code edits were made, so the delegated build and focused backend CTest
  subset were not required for this Step 4 documentation/classification slice.
- Existing `test_after.log` remains the prior Step 3 proof log and was not
  rewritten by this no-code packet.
