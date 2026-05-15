Status: Active
Source Idea Path: ideas/open/241_aarch64_crc_vector_intrinsic_carriers.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory accepted intrinsic families and carrier fields

# Current Packet

## Just Finished

Lifecycle activation only. No executor packet has completed yet.

## Suggested Next

Start `plan.md` Step 1 by inventorying the existing scalar `fabs` path and
defining the minimum structured BIR/prepared fields for CRC, vector memory, and
vector operation carrier representatives.

## Watchouts

- Do not select or print AArch64 CRC/vector machine records in this dependency
  route.
- Do not use intrinsic-name matching, ordinary call plans alone, archived
  scratch registers, or final assembly text as carrier authority.
- Keep unsupported x86-only and incomplete-fact paths fail-closed.

## Proof

Lifecycle proof required before handoff:

- lifecycle invariant checks
- `git diff --check`
