Status: Active
Source Idea Path: ideas/open/429_rv64_pointer_address_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Classify Pointer/Address Evidence And Choose First Packet

# Current Packet

## Just Finished

Activated `ideas/open/429_rv64_pointer_address_materialization.md` as the next
RV64 post-contract implementation plan. Selection rationale: the follow-up
ordering places pointer/address materialization after the closed scalar
call-adjacent inline-asm idea, and the source evidence names a 21-row ordinary
C bucket with explicit producer-gap boundaries.

## Suggested Next

Execute Step 1: classify the pointer/address representative rows into coherent
`inttoptr`, coherent `ptrtoint`, address-bearing scalar materialization,
global/addressability residual, and missing-provenance producer-gap buckets,
then choose the first implementation packet.

## Watchouts

- Do not infer object identity, provenance, relocation base, or addressability
  from integer shape, source filenames, raw BIR, or representative row text.
- Keep global memory/addressing residual rows separate unless prepared facts
  prove they are ordinary address materialization within this idea.
- If the first packet exposes a BIR/prepared provenance gap, stop and request a
  lifecycle split instead of patching RV64 lowering.
- Do not change expectations, allowlists, unsupported markers, runtime
  comparison, or pass/fail accounting.

## Proof

Activation-only lifecycle change. Required local check:

```sh
git diff --check
```
