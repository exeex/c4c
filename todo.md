Status: Active
Source Idea Path: ideas/open/352_aarch64_block_label_emission_ordering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Block Emission Boundary

# Current Packet

## Just Finished

Lifecycle switched from parked idea 349 to active idea 352 after `00176`
advanced past stale call-preservation publication and exposed a new AArch64
block label/emission ordering first bad fact.

## Suggested Next

Execute plan Step 1: localize where prepared `partition` `block_3` return
metadata and later reachable `block_4`/`block_5` code become emitted as a
return/epilogue followed by unlabeled executable `swap` code.

## Watchouts

- Do not rework call-argument publication unless fresh evidence shows another
  stale caller-clobbered post-call argument use.
- Do not special-case `00176`, `partition`, one block id, one label suffix, or
  one emitted return sequence.
- Keep `00181` out of this route unless its current stack-preserved
  symbol/local publication crash is fixed or bypassed by its owner and a fresh
  block-label first bad fact appears.

## Proof

Lifecycle-only switch. Existing context from the parked 349 packet:

- focused proof remained 4/6 with `00176` and `00181` still failing;
- `00176` now emits `mov x0, x20` before the final `bl swap` and no generated
  `mov w0, w1`, advancing past stale call-preservation publication;
- remaining `00176` first bad fact appears to be generated AArch64 block
  label/emission ordering after `partition` `block_3` return;
- `00181` remains at the out-of-scope stack-preserved symbol/local publication
  crash;
- supervisor broader backend guard passed 141/141.
