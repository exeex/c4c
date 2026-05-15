Status: Active
Source Idea Path: ideas/open/242_aarch64_barrier_cache_hint_builtin_intrinsic_carriers.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Current Carrier Gaps

# Current Packet

## Just Finished

Lifecycle activation only. No executor packet has completed yet.

## Suggested Next

Start `plan.md` Step 1 by inventorying the current BIR and prepared intrinsic
carrier gaps for barrier, cache-maintenance, pause/hint, and builtin-address
representatives.

## Watchouts

- Do not select or print AArch64 machine records for these families in this
  carrier plan.
- Keep atomic fence carriers and prepared address materialization distinct from
  intrinsic-family carrier authority.
- Treat x86-only, target-mismatched, malformed, missing-home, and incomplete
  paths as fail-closed diagnostics.
- Preserve existing scalar, CRC, and vector intrinsic behavior while extending
  the carrier surfaces.

## Proof

Lifecycle proof required before handoff:

- lifecycle invariant checks
- `git diff --check`
