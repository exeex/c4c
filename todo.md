Status: Active
Source Idea Path: ideas/open/258_phase_f3_x86_route3_loadlocal_source_memory_agreement_bridge.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Focused Agreement and Rejection Proof

# Current Packet

## Just Finished

Lifecycle resumed idea 258 after closing idea 261.

Idea 261 provides the supported joined-branch compare-join `EdgeStoreSlot`
selected-`LoadLocal` proof surface needed for this plan's Step 4. Use that
production route to prove the x86 Route 3/prepared source-memory agreement
facade; do not return to synthetic `join_transfers` on routes that reject them
before the facade.

## Suggested Next

Execute plan Step 4, `Focused Agreement and Rejection Proof`.

Start from the existing selected-`LoadLocal` joined-branch rows and prove the
positive agreement path plus naturally reachable fail-closed rows: missing
source address, join-carrier-only drift, and incomplete prepared source-memory
publication. Record prepared-only, stale-publication, byte-offset drift, or
cross-publication mismatch rows as unsupported by this surface unless a
supported route can express them without synthetic or stale prepared state.

## Watchouts

- Do not count legacy no-publication fallback as positive Route 3/prepared
  agreement proof.
- Do not hand-build `PreparedEdgePublication` rows, stale prepared render
  contract state, or named-case shortcuts.
- Preserve prepared lookup/status behavior, helper/oracle names, fallback
  names, route-debug output, and x86 output spelling.

## Proof

Close/resume validation used the existing executor logs. Regression guard
passed with `test_before.log` and `test_after.log`; `test_after.log` shows 2/2
default prepared tests and 2/2 x86 tests passed.
